#include "LogicSystem.h"
#include <iostream>
#include <json/json.h>
#include <map>
#include <mutex>
#include <functional>
#include "StatusGrpcClient.h" 
#include "MysqlManager.h"     
#include "const.h"
#include "RedisManager.h"
#include "UserManager.h"
#include "ChatGrpcClient.h"
namespace {
    std::map<int, std::shared_ptr<UserInfo>> g_users;
    std::mutex g_users_mtx;
}

bool GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo) {
    std::cout << "[追踪] GetBaseInfo 开始执行, UID: " << uid << std::endl;
    std::lock_guard<std::mutex> lock(g_users_mtx);
    std::string info_str = "";
    bool success = RedisManager::GetInstance()->Get(base_key, info_str);
    if (userinfo == nullptr) {
        userinfo = std::make_shared<UserInfo>();
    }
    if (success) {
        std::cout << "[追踪] 命中 Redis 缓存" << std::endl;
        Json::Reader reader;
        Json::Value root;
        reader.parse(info_str, root);
        userinfo->uid = root["uid"].asInt();
        userinfo->name = root["name"].asString();
        userinfo->password = root["password"].asString();
        userinfo->email = root["email"].asString();
        userinfo->nick = root["nick"].asString();
        userinfo->desc = root["desc"].asString();
        userinfo->sex = root["sex"].asInt();
        userinfo->icon = root["icon"].asString();
    }
    else {
        std::cout << "[追踪] Redis 缓存未命中，开始查询 MySQL..." << std::endl;
        std::shared_ptr<UserInfo> user_info = nullptr;
        user_info = MysqlManager::GetInstance()->GetUser(uid);
        if (user_info == nullptr) {
            std::cout << "[错误] MySQL 查询不到该用户信息！" << std::endl;
            return false;
        }
        userinfo = user_info;
        Json::Value redis_root;
        redis_root["uid"] = userinfo->uid;
        redis_root["name"] = userinfo->name;
        redis_root["password"] = userinfo->password;
        redis_root["email"] = userinfo->email;
        redis_root["nick"] = userinfo->nick;
        redis_root["desc"] = userinfo->desc;
        redis_root["sex"] = userinfo->sex;
        redis_root["icon"] = userinfo->icon;
        RedisManager::GetInstance()->Set(base_key, redis_root.toStyledString());
        std::cout << "[追踪] MySQL 数据已回写至 Redis" << std::endl;
    }

    g_users[uid] = userinfo;
    std::cout << "[追踪] GetBaseInfo 执行完毕，成功返回" << std::endl;
    return true;
}

bool GetFriendApplyInfo(int to_uid, std::vector<std::shared_ptr<ApplyInfo>>& list) {
    //从mysql获取好友申请列表
    return MysqlManager::GetInstance()->GetApplyList(to_uid, list, 0, 10);
}

bool GetFriendList(int to_uid, std::vector<std::shared_ptr<UserInfo>>& friend_list) {
	return MysqlManager::GetInstance()->GetFriendList(to_uid, friend_list);
}

bool isPureDigit(const std::string& str) {
    if (str.empty()) return false;
    for (char c : str) {
        if (!std::isdigit(c)) return false;
    }
    return true;
}

// 通过 UID 搜索
void GetUserByUid(const std::string& uid_str, Json::Value& rtvalue) {
    rtvalue["error"] = ErrorCodes::Success;

    std::string base_key = USER_BASE_INFO + uid_str;
    std::string info_str = "";

    // 第一步：优先去 Redis 查
    bool b_base = RedisManager::GetInstance()->Get(base_key, info_str);
    if (b_base) {
        std::cout << "[追踪] GetUserByUid 命中 Redis 缓存" << std::endl;
        Json::Reader reader;
        Json::Value root;
        reader.parse(info_str, root);

        rtvalue["uid"] = root["uid"].asInt();
        rtvalue["name"] = root["name"].asString();
        rtvalue["nick"] = root["nick"].asString();
        rtvalue["desc"] = root["desc"].asString();
        rtvalue["sex"] = root["sex"].asInt();
        rtvalue["icon"] = root["icon"].asString();
        return; // 缓存命中，直接返回，不再打扰 MySQL
    }

    // 第二步：Redis 没查到，去查 MySQL
    std::cout << "[追踪] GetUserByUid 缓存未命中，开始查询 MySQL..." << std::endl;
    std::shared_ptr<UserInfo> user_info = MysqlManager::GetInstance()->GetUser(uid_str);

    if (user_info != nullptr) {
        rtvalue["uid"] = user_info->uid;
        rtvalue["name"] = user_info->name;
        rtvalue["nick"] = user_info->nick;
        rtvalue["desc"] = user_info->desc;
        rtvalue["sex"] = user_info->sex;
        rtvalue["icon"] = user_info->icon;

        // 第三步：把 MySQL 查到的数据写回 Redis，方便下次查询
        Json::Value redis_root;
        redis_root["uid"] = user_info->uid;
        redis_root["name"] = user_info->name;
        redis_root["nick"] = user_info->nick;
        redis_root["desc"] = user_info->desc;
        redis_root["sex"] = user_info->sex;
        redis_root["icon"] = user_info->icon;

        RedisManager::GetInstance()->Set(base_key, redis_root.toStyledString());
        std::cout << "[追踪] MySQL 数据已回写至 Redis (Key: " << base_key << ")" << std::endl;
    }
    else {
        rtvalue["error"] = ErrorCodes::UidInvalid; // 查无此人
    }
}

// 通过用户名搜索
void GetUserByName(const std::string& name, Json::Value& rtvalue) {
    rtvalue["error"] = ErrorCodes::Success;

    std::string base_key = NAME_INFO + name;
    std::string info_str = "";

    // 第一步：优先去 Redis 查
    bool b_base = RedisManager::GetInstance()->Get(base_key, info_str);
    if (b_base) {
        std::cout << "[追踪] GetUserByName 命中 Redis 缓存" << std::endl;
        Json::Reader reader;
        Json::Value root;
        reader.parse(info_str, root);

        rtvalue["uid"] = root["uid"].asInt();
        rtvalue["name"] = root["name"].asString();
        rtvalue["nick"] = root["nick"].asString();
        rtvalue["desc"] = root["desc"].asString();
        rtvalue["sex"] = root["sex"].asInt();
        rtvalue["icon"] = root["icon"].asString();
        return; // 缓存命中，直接返回，不再打扰 MySQL
    }

    // 第二步：Redis 没查到，去查 MySQL
    std::cout << "[追踪] GetUserByName 缓存未命中，开始查询 MySQL..." << std::endl;
    std::shared_ptr<UserInfo> user_info = MysqlManager::GetInstance()->GetUser(name);

    if (user_info != nullptr) {
        rtvalue["uid"] = user_info->uid;
        rtvalue["name"] = user_info->name;
        rtvalue["nick"] = user_info->nick;
        rtvalue["desc"] = user_info->desc;
        rtvalue["sex"] = user_info->sex;
        rtvalue["icon"] = user_info->icon;

        // 第三步：把 MySQL 查到的数据写回 Redis，方便下次查询
        Json::Value redis_root;
        redis_root["uid"] = user_info->uid;
        redis_root["name"] = user_info->name;
        redis_root["nick"] = user_info->nick;
        redis_root["desc"] = user_info->desc;
        redis_root["sex"] = user_info->sex;
        redis_root["icon"] = user_info->icon;

        RedisManager::GetInstance()->Set(base_key, redis_root.toStyledString());
        std::cout << "[追踪] MySQL 数据已回写至 Redis (Key: " << base_key << ")" << std::endl;
    }
    else {
        rtvalue["error"] = ErrorCodes::UidInvalid; // 查无此人
    }
}

void NotifyOffline(std::shared_ptr<Session> target_session,int uid) {
    if (!target_session) return;
    Json::Value  rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["uid"] = uid;


    std::string return_str = rtvalue.toStyledString();

    target_session->Send(return_str, ID_NOTIFY_OFF_LINE_REQ);
    return;
}

void ChatLoginHandler(std::shared_ptr<Session> session, short msg_id, std::string msg_data) {
    std::cout << "\n========== [聊天登录流程开始] ==========" << std::endl;
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data, root);

    int uid = root["uid"].asInt();
    std::string token = root["token"].asString();
    std::cout << "[追踪] 收到 TCP 登录请求, UID: " << uid << " | Token: " << token << std::endl;

    Json::Value rtvalue;

    Defer defer([&session, &rtvalue, msg_id]() {
        std::string return_str = rtvalue.toStyledString();
        std::cout << "[追踪] 触发 Defer，准备向客户端发送最终回包: " << return_str << std::endl;
        session->Send(return_str, MSG_CHAT_LOGIN_REP);
        std::cout << "========== [聊天登录流程结束] ==========\n" << std::endl;
        });

    std::string uid_str = std::to_string(uid);
    std::string token_key = USERTOKENPREFIX + uid_str;
    std::string token_value = "";

    std::cout << "[追踪] 正在 Redis 校验 Token..." << std::endl;
    bool success = RedisManager::GetInstance()->Get(token_key, token_value);
    if (!success) {
        std::cout << "[错误] Redis 中找不到 Token 记录" << std::endl;
        rtvalue["error"] = ErrorCodes::UidInvalid;
        return;
    }
    if (token_value != token) {
        std::cout << "[错误] Token 不匹配！客户端: " << token << " | 服务端: " << token_value << std::endl;
        rtvalue["error"] = ErrorCodes::TokenInvalid;
        return;
    }
    std::cout << "[追踪] Token 校验通过！" << std::endl;

    rtvalue["error"] = ErrorCodes::Success;
    std::string base_key = USER_BASE_INFO + uid_str;
    auto user_info = std::make_shared<UserInfo>();

    bool base_info_success = GetBaseInfo(base_key, uid, user_info);
    if (!base_info_success) {
        std::cout << "[错误] 获取基础信息 (GetBaseInfo) 失败" << std::endl;
        rtvalue["error"] = ErrorCodes::UidInvalid;
        return;
    }

    rtvalue["uid"] = uid;
    rtvalue["token"] = token;
    rtvalue["name"] = user_info->name;
    rtvalue["email"] = user_info->email;
    rtvalue["nick"] = user_info->nick;
    rtvalue["desc"] = user_info->desc;
    rtvalue["sex"] = user_info->sex;
    rtvalue["icon"] = user_info->icon;
    rtvalue["applylist"] = Json::arrayValue;

    std::vector<std::shared_ptr<ApplyInfo>> apply_list;
    bool apply_success = GetFriendApplyInfo(uid, apply_list);
    if (!apply_success) {
        std::cout << "[错误] 获取好友申请列表 (GetFriendApplyInfo) 失败" << std::endl;
        rtvalue["error"] = ErrorCodes::UidInvalid;
        return;
    }
    for (auto& apply : apply_list) {
        Json::Value apply_json;
        apply_json["applyuid"] = apply->_uid;
        apply_json["name"] = apply->_name;
        apply_json["desc"] = apply->_desc;
        apply_json["icon"] = apply->_icon;
        apply_json["nick"] = apply->_nick;
        apply_json["sex"] = apply->_sex;
        apply_json["status"] = apply->_status;
        rtvalue["apply_list"].append(apply_json);
    }

    //获取好友列表
    rtvalue["friendlist"] = Json::arrayValue;
    std::vector<std::shared_ptr<UserInfo>> friend_list;
    bool b_friend_list = GetFriendList(uid, friend_list);
    for (auto& friend_ele : friend_list) {
        Json::Value obj;
        obj["name"] = friend_ele->name;
        obj["uid"] = friend_ele->uid;
        obj["icon"] = friend_ele->icon;
        obj["nick"] = friend_ele->nick;
        obj["sex"] = friend_ele->sex;
        obj["desc"] = friend_ele->desc;
        obj["back"] = friend_ele->back;
        rtvalue["friend_list"].append(obj);
    }

	auto lockKey = LOCK_PREFIX + uid_str;
	auto identifier = RedisManager::GetInstance()->acquireLock(lockKey, LOCK_TIME_OUT, ACQUIRE_TIME_OUT);

    if (identifier.empty()) {
        std::cout << "[警告] 无法获取分布式锁，UID: " << uid << " 正在被其他线程处理" << std::endl;
        rtvalue["error"] = ErrorCodes::UserExist; // 请替换为你系统里实际的“系统繁忙”错误码
        return; // 直接 return，最上面的 defer 会帮你给客户端发回包
    }

    Defer lockDefer([lockKey, identifier]() {
        if (!identifier.empty()) {
            RedisManager::GetInstance()->releaseLock(lockKey, identifier);
            std::cout << "[追踪] 已释放 Redis 分布式锁" << std::endl;
        }
		});

    std::string uid_ip_key = "";
	auto uid_ip_value = USERIPPREFIX + uid_str;
	bool b_ip = RedisManager::GetInstance()->Get(uid_ip_key, uid_ip_value);
    if (b_ip) {
		auto server_name = ConfigManager::Inst().GetValue("SelfServer", "Name");
        if (server_name == uid_ip_value) {
			auto old_session = UserManager::GetInstance()->GetUserSession(uid);
            if (old_session) {
                NotifyOffline(old_session,uid);
                //清除旧的连接
                old_session->GetServer()->ClearSession(old_session->GetSessionId());
            }
        }
    }

    std::cout << "[追踪] 正在登记在线状态到 Redis..." << std::endl;
    auto server_name = ConfigManager::Inst().GetValue("SelfServer", "Name");
    auto redis_res = RedisManager::GetInstance()->HGet(LOGIN_COUNT, server_name);
    int count = 0;
    if (!redis_res.empty()) {
        count = std::stoi(redis_res);
    }
    count++;
    auto count_str = std::to_string(count);
    RedisManager::GetInstance()->HSet(LOGIN_COUNT, server_name, count_str);

    session->SetUserId(uid);
    std::string ip_key = USERIPPREFIX + uid_str;
    RedisManager::GetInstance()->Set(ip_key, server_name);
    UserManager::GetInstance()->SetUserSession(uid, session);

    std::cout << "[追踪] 登录逻辑全部执行完毕，准备返回 (即将触发 Defer)" << std::endl;
    return;
}

void ChatSearchHandler(std::shared_ptr<Session> session, short msg_id, std::string msg_data) {
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data, root);

    auto uid_str = root["uid"].asString();
    Json::Value rtvalue;

    // 无论下面逻辑如何 return，退出时必然发送回包
    Defer defer([&session, &rtvalue]() {
        std::string return_str = rtvalue.toStyledString();
        session->Send(return_str, ID_SEARCH_USER_RSP);
        });

    bool b_digit = isPureDigit(uid_str);
    if (b_digit) {
        GetUserByUid(uid_str, rtvalue);
    }
    else {
        GetUserByName(uid_str, rtvalue);
    }
}

void AddFriendHandler(std::shared_ptr<Session> session, short msg_id, std::string msg_data) {
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data, root);

    auto uid = root["uid"].asInt();
	auto applyname = root["applyname"].asString();
	auto backname = root["backname"].asString();
	auto touid = root["touid"].asInt();

    Json::Value rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    // 无论下面逻辑如何 return，退出时必然发送回包
    Defer defer([&session, &rtvalue]() {
        std::string return_str = rtvalue.toStyledString();
        session->Send(return_str, ID_ADD_FRIEND_RSP);
        });

	MysqlManager::GetInstance()->AddFriend(uid,touid);

    auto base_key = USER_BASE_INFO + std::to_string(uid);
    std::shared_ptr<UserInfo> apply_info = std::make_shared<UserInfo>();
    bool b_info = GetBaseInfo(base_key, uid, apply_info);
	auto to_str = std::to_string(touid);
	auto to_ip_key = USERIPPREFIX + to_str;

	std::string to_ip_value = "";
	bool b_ip = RedisManager::GetInstance()->Get(to_ip_key, to_ip_value);
    if (!b_ip) {
        return;
    }
    auto server_name = ConfigManager::Inst().GetValue("SelfServer", "Name");
    if (server_name == to_ip_value) {
		auto target_session = UserManager::GetInstance()->GetSession(touid);
        if (target_session) {
            Json::Value  notify;
            notify["error"] = ErrorCodes::Success;
            notify["applyuid"] = uid;
            notify["name"] = applyname;
            notify["desc"] = "";
            if (b_info) {
                notify["icon"] = apply_info->icon;
				notify["sex"] = apply_info->sex;
				notify["nick"] = apply_info->nick;
            }
            std::string return_str = notify.toStyledString();
            target_session->Send(return_str, ID_NOTIFY_ADD_FRIEND_REQ);
        }
    }

    AddFriendReq add_req;
    add_req.set_applyuid(uid);
    add_req.set_touid(touid);
    add_req.set_name(applyname);
    add_req.set_desc("");
    if (b_info) {
        add_req.set_icon(apply_info->icon);
        add_req.set_sex(apply_info->sex);
        add_req.set_nick(apply_info->nick);
    }

	ChatGrpcClient::GetInstance()->AddFriend(to_ip_value, add_req);
}

void AuthFriendHandler(std::shared_ptr<Session> session, short msg_id, std::string msg_data) {
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data, root);

    auto uid = root["fromuid"].asInt();
    auto backname = root["back"].asString();
    auto touid = root["touid"].asInt();

    Json::Value rtvalue;
	rtvalue["error"] = ErrorCodes::Success;
	auto user_info = std::make_shared<UserInfo>();
	bool base_info_success = GetBaseInfo(USER_BASE_INFO + std::to_string(touid), touid, user_info);
    if (base_info_success) {
        rtvalue["applyuid"] = touid;
		rtvalue["name"] = user_info->name;
        rtvalue["nick"] = user_info->nick;
        rtvalue["desc"] = user_info->desc;
		rtvalue["sex"] = user_info->sex;
		rtvalue["icon"] = user_info->icon;
    }
    else {
        rtvalue["error"] = ErrorCodes::UidInvalid;
		return;
    }
    // 无论下面逻辑如何 return，退出时必然发送回包
    Defer defer([&session, &rtvalue]() {
        std::string return_str = rtvalue.toStyledString();
        session->Send(return_str, ID_AUTH_FRIEND_RSP);
        });

    MysqlManager::GetInstance()->AuthFriend(uid, touid,backname);

    auto to_str = std::to_string(touid);
    auto to_ip_key = USERIPPREFIX + to_str;
    std::string to_ip_value = "";
    bool b_ip = RedisManager::GetInstance()->Get(to_ip_key, to_ip_value);
    if (!b_ip) {
        return;
    }
    auto server_name = ConfigManager::Inst().GetValue("SelfServer", "Name");
	//在同一台服务器（内存中）上，直接发送通知
    if (server_name == to_ip_value) {
        auto session = UserManager::GetInstance()->GetSession(touid);
        if (session) {
            Json::Value notify;
            notify["error"] = ErrorCodes::Success;
            notify["applyuid"] = uid;
            notify["desc"] = "";
            std::string base_key = USER_BASE_INFO + std::to_string(uid);
            auto user_info = std::make_shared<UserInfo>();
            bool b_info = GetBaseInfo(base_key, uid, user_info);
            if (b_info) {
                notify["name"] = user_info->name;
                notify["nick"] = user_info->nick;
                notify["icon"] = user_info->icon;
                notify["sex"] = user_info->sex;
            }
            else {
                notify["error"] = ErrorCodes::UidInvalid;
            }
            std::string return_str = notify.toStyledString();
            session->Send(return_str, ID_NOTIFY_AUTH_FRIEND_REQ);
        }
        return;
    }
    
    AuthFriendReq auth_req;
    auth_req.set_fromuid(uid);
    auth_req.set_touid(touid);

    ChatGrpcClient::GetInstance()->AuthFriend(to_ip_value, auth_req);
}

void ChatTextHandler(std::shared_ptr<Session> session, short msg_id, std::string msg_data) {
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data, root);
    auto uid = root["uid"].asInt();
    auto touid = root["touid"].asInt();
    Json::Value text_array = root["text_array"];

    Json::Value rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
	rtvalue["fromuid"] = uid;
	rtvalue["touid"] = touid;
	rtvalue["text_array"] = text_array;
    // 无论下面逻辑如何 return，退出时必然发送回包
    Defer defer([&session, &rtvalue]() {
        std::string return_str = rtvalue.toStyledString();
        session->Send(return_str, ID_TEXT_CHAT_MSG_RSP);
        });

    auto to_str = std::to_string(touid);
    auto to_ip_key = USERIPPREFIX + to_str;
    std::string to_ip_value = "";
    bool b_ip = RedisManager::GetInstance()->Get(to_ip_key, to_ip_value);
    if (!b_ip) {
        rtvalue["error"] = ErrorCodes::UidInvalid;
        return;
    }
    
    auto server_name = ConfigManager::Inst().GetValue("SelfServer", "Name");
    //在同一台服务器（内存中）上，直接发送通知
    if (server_name == to_ip_value) {
        auto target_session = UserManager::GetInstance()->GetSession(touid);
        if (target_session) {
            std::string return_str = rtvalue.toStyledString();
            target_session->Send(return_str, ID_NOTIFY_TEXT_CHAT_MSG_REQ);
        }
        return;
    }

    TextChatMsgReq chat_req;
    chat_req.set_fromuid(uid);
    chat_req.set_touid(touid);
    for (const auto& text : text_array) {
		auto content = text["content"].asString();
		auto msg_id = text["msg_id"].asInt();
		auto* text_msg = chat_req.add_textmsgs();
		text_msg->set_msgcontext(content);
		text_msg->set_msgid(msg_id);
    }
    ChatGrpcClient::GetInstance()->TextChatMsg(to_ip_value, chat_req,rtvalue);
}

void HeartBeatHandler(std::shared_ptr<Session> session, const short& msg_id, const std::string& msg_data) {
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data, root);
    auto uid = root["fromuid"].asInt();
    std::cout << "receive heart beat msg, uid is " << uid << std::endl;
    Json::Value rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    Defer defer([&session, &rtvalue]() {
        session->Send(rtvalue.toStyledString(), ID_TEXT_CHAT_MSG_RSP);
        });
}

void GetUserThreadsHandler(std::shared_ptr<Session> session, short msg_id, std::string msg_data) {
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data, root);

    auto uid = root["uid"].asInt64();
    auto last_id = root.isMember("last_id") ? root["last_id"].asInt64() : 0; // 首次拉取传 0
    auto page_size = root.isMember("page_size") ? root["page_size"].asInt() : 20; // 默认一页20条

    Json::Value rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["uid"] = (int)uid;

    Defer defer([&session, &rtvalue]() {
        session->Send(rtvalue.toStyledString(), ID_LOAD_CHAT_THREAD_RSP);
        });

    std::vector<std::shared_ptr<ChatThreadInfo>> threads;
    bool load_more = false;
    int64_t next_last_id = 0;

    // 通过 MysqlManager 调用刚才优化的 DAO 方法
    bool res = MysqlManager::GetInstance()->GetUserThreads(uid, last_id, page_size, threads, load_more, next_last_id);
    if (!res) {
        rtvalue["error"] = ErrorCodes::UidInvalid;
        return;
    }

    rtvalue["load_more"] = load_more;
    rtvalue["next_last_id"] = (Json::Int64)next_last_id;
    rtvalue["threads"] = Json::arrayValue;

    for (auto& thread : threads) {
        Json::Value thread_value;
        thread_value["thread_id"] = (Json::Int64)thread->_thread_id;
        thread_value["type"] = thread->_type;
        thread_value["user1_id"] = (Json::Int64)thread->_user1_id;
        thread_value["user2_id"] = (Json::Int64)thread->_user2_id;
        rtvalue["threads"].append(thread_value);
    }
}

REGISTER_CALL_BACK(ID_LOAD_CHAT_THREAD_REQ, GetUserThreadsHandler);
REGISTER_CALL_BACK(ID_HEART_BEAT_REQ, HeartBeatHandler)
REGISTER_CALL_BACK(ID_TEXT_CHAT_MSG_REQ, ChatTextHandler)
REGISTER_CALL_BACK(ID_AUTH_FRIEND_REQ, AuthFriendHandler)
REGISTER_CALL_BACK(ID_ADD_FRIEND_REQ, AddFriendHandler)
REGISTER_CALL_BACK(ID_SEARCH_USER_REQ, ChatSearchHandler);
REGISTER_CALL_BACK(MSG_CHAT_LOGIN, ChatLoginHandler);