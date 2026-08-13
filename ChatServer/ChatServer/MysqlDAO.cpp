#include "MysqlDAO.h"
#include "ConfigManager.h"

MysqlDAO::MysqlDAO()
{
    auto& cfg = ConfigManager::Inst();
    const auto& host = cfg["Mysql"]["Host"];
    const auto& port = cfg["Mysql"]["Port"];
    const auto& pwd = cfg["Mysql"]["Password"];
    const auto& schema = cfg["Mysql"]["Schema"];
    const auto& user = cfg["Mysql"]["User"];
    pool_.reset(new MySqlPool("tcp://" + host + ":" + port, user, pwd, schema, 5));
}

MysqlDAO::~MysqlDAO() {
    pool_->Close();
}

int MysqlDAO::RegUser(const std::string& name, const std::string& email, const std::string& pwd)
{
    auto con = pool_->getConnection();
    try {
        if (con == nullptr) {
            return false;
        }
        std::unique_ptr < sql::PreparedStatement > stmt(con->_con->prepareStatement("CALL reg_user(?,?,?,@result)"));
        stmt->setString(1, name);
        stmt->setString(2, email);
        stmt->setString(3, pwd);

        stmt->execute();
        std::unique_ptr<sql::Statement> stmtResult(con->_con->createStatement());
        std::unique_ptr<sql::ResultSet> res(stmtResult->executeQuery("SELECT @result AS result"));
        if (res->next()) {
            int result = res->getInt("result");
            std::cout << "Result: " << result << "\n";
            pool_->returnConnection(std::move(con));
            return result;
        }
        pool_->returnConnection(std::move(con));
        return -1;
    }
    catch (sql::SQLException& e) {
        pool_->returnConnection(std::move(con));
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return -1;
    }
}

bool MysqlDAO::CheckEmail(const std::string& name, const std::string& email) {
    auto con = pool_->getConnection();
    try {
        if (con == nullptr) {
            pool_->returnConnection(std::move(con));
            return false;
        }

        std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("SELECT email FROM user WHERE name = ?"));

        pstmt->setString(1, name);

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

        while (res->next()) {
            std::cout << "Check Email: " << res->getString("email") << std::endl;
            if (email != res->getString("email")) {
                pool_->returnConnection(std::move(con));
                return false;
            }
            pool_->returnConnection(std::move(con));
            return true;
        }
        pool_->returnConnection(std::move(con));
        return false;
    }
    catch (sql::SQLException& e) {
        pool_->returnConnection(std::move(con));
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}

bool MysqlDAO::UpdatePwd(const std::string& name, const std::string& newpwd) {
    auto con = pool_->getConnection();
    try {
        if (con == nullptr) {
            pool_->returnConnection(std::move(con));
            return false;
        }

        std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("UPDATE user SET password = ? WHERE name = ?"));

        pstmt->setString(2, name);
        pstmt->setString(1, newpwd);

        int updateCount = pstmt->executeUpdate();

        std::cout << "Updated rows: " << updateCount << std::endl;
        pool_->returnConnection(std::move(con));
        return true;
    }
    catch (sql::SQLException& e) {
        pool_->returnConnection(std::move(con));
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}

bool MysqlDAO::CheckPwd(const std::string& name, const std::string& pwd, UserInfo& userInfo) {
    auto con = pool_->getConnection();
    try {
        if (con == nullptr) {
            pool_->returnConnection(std::move(con));
            return false;
        }

        std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("SELECT * FROM user WHERE email = ?"));
        pstmt->setString(1, name);

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        std::string origin_pwd = "";

        if (res->next()) {
            origin_pwd = res->getString("password");
            std::cout << "Password: " << origin_pwd << std::endl;

            if (pwd != origin_pwd) {
                pool_->returnConnection(std::move(con));
                return false;
            }

            userInfo.name = res->getString("name");
            userInfo.email = res->getString("email");
            userInfo.uid = res->getInt("uid");
            userInfo.password = origin_pwd;

            pool_->returnConnection(std::move(con));
            return true;
        }
        else {
            pool_->returnConnection(std::move(con));
            return false;
        }
    }
    catch (sql::SQLException& e) {
        pool_->returnConnection(std::move(con));
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}

std::shared_ptr<UserInfo> MysqlDAO::GetUser(int uid)
{
    auto con = pool_->getConnection();
    if (con == nullptr) {
        return nullptr;
    }

    Defer defer([this, &con]() {
        pool_->returnConnection(std::move(con));
        });

    try {
        std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("SELECT * FROM user WHERE uid = ?"));
        pstmt->setInt(1, uid); 

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        std::shared_ptr<UserInfo> user_ptr = nullptr;
        while (res->next()) {
            user_ptr.reset(new UserInfo);
            user_ptr->password = res->getString("password");
            user_ptr->email = res->getString("email");
            user_ptr->name = res->getString("name");
            user_ptr->uid = res->getInt("uid");
            user_ptr->nick = res->getString("nick");
            user_ptr->desc = res->getString("desc");
            user_ptr->sex = res->getInt("sex");
            user_ptr->icon = res->getString("icon");
            break;
        }
        return user_ptr;
    }
    catch (sql::SQLException& e) {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return nullptr;
    }
}

std::shared_ptr<UserInfo> MysqlDAO::GetUser(std::string name)
{
    auto con = pool_->getConnection();
    if (con == nullptr) {
        return nullptr;
    }

    Defer defer([this, &con]() {
        pool_->returnConnection(std::move(con));
        });

    try {
        std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("SELECT * FROM user WHERE name = ?"));
        pstmt->setString(1, name);

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        std::shared_ptr<UserInfo> user_ptr = nullptr;
        while (res->next()) {
            user_ptr.reset(new UserInfo);
            user_ptr->password = res->getString("password");
            user_ptr->email = res->getString("email");
            user_ptr->name = res->getString("name");
            user_ptr->uid = res->getInt("uid");
			user_ptr->nick = res->getString("nick");
			user_ptr->desc = res->getString("desc");
			user_ptr->sex = res->getInt("sex");
			user_ptr->icon = res->getString("icon");
            break;
        }
        return user_ptr;
    }
    catch (sql::SQLException& e) {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return nullptr;
    }
}

bool MysqlDAO::GetApplyList(int touid, std::vector<std::shared_ptr<ApplyInfo>>& applyList, int begin, int limit)
{
    auto con = pool_->getConnection();
    if (con == nullptr) {
        return false;
    }

    Defer defer([this, &con]() {
        pool_->returnConnection(std::move(con));
        });

    try {
        std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("select apply.from_uid, apply.status, user.name, "
            "user.nick, user.sex from friend_apply as apply join user on apply.from_uid = user.uid where apply.to_uid = ? "
            "and apply.id > ? order by apply.id ASC LIMIT ? "));

        pstmt->setInt(1, touid); // 将uid替换为你要查询的uid
        pstmt->setInt(2, begin); // 起始id
        pstmt->setInt(3, limit); //偏移量
        // 执行查询
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        // 遍历结果集
        while (res->next()) {
            auto name = res->getString("name");
            auto uid = res->getInt("from_uid");
            auto status = res->getInt("status");
            auto nick = res->getString("nick");
            auto sex = res->getInt("sex");
            auto apply_ptr = std::make_shared<ApplyInfo>(uid, name, "", "", nick, sex, status);
            applyList.push_back(apply_ptr);
        }
        return true;
    }
    catch (sql::SQLException& e) {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}

bool MysqlDAO::GetFriendList(int to_uid, std::vector<std::shared_ptr<UserInfo>>& friend_list) {
    auto con = pool_->getConnection();
    if (con == nullptr) {
        return false;
    }

    Defer defer([this, &con]() {
        pool_->returnConnection(std::move(con));
        });
    
    try {
        // 准备SQL语句, 根据起始id和限制条数返回列表
        std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("select * from friend where self_id = ? "));

        pstmt->setInt(1, to_uid); // 将uid替换为你要查询的uid

        // 执行查询
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        // 遍历结果集
        while (res->next()) {
            auto friend_id = res->getInt("friend_id");
            std::string back = res->getString("back");
            //再一次查询friend_id对应的信息
            auto user_info = GetUser(friend_id);
            if (user_info == nullptr) {
                continue;
            }

            user_info->back == " " ? user_info->name : back;
            friend_list.push_back(user_info);
        }
        return true;
    }
    catch (sql::SQLException& e) {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }

    return true;
}

bool MysqlDAO::AddFriend(const int& from, const int& to) {
    auto con = pool_->getConnection();
    if (con == nullptr) {
        return false;
    }

    Defer defer([this, &con]() {
        pool_->returnConnection(std::move(con));
        });

    try {
        std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("INSERT INTO friend_apply (from_uid, to_uid) VALUES (?, ?)"
        "ON DUPLICATE KEY UPDATE from_uid = from_uid,to_uid = to_uid "));
        pstmt->setInt(1, from);
        pstmt->setInt(2, to);
        int updateCount = pstmt->executeUpdate();
        if (updateCount < 0) {
            return false;
        }
        std::cout << "Added friend, rows affected: " << updateCount << std::endl;
        return true;
    }
    catch (sql::SQLException& e) {
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
    }
}

bool MysqlDAO::AuthFriend(const int& from, const int& to, std::string& backname) {
    auto con = pool_->getConnection();
    if (con == nullptr) {
        return false;
    }

    Defer defer([this, &con]() {
        pool_->returnConnection(std::move(con));
        });

    {
        std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("UPDATE friend_apply SET status = 1 "
            "WHERE from_uid = ? AND to_uid = ?"));
        //反过来的申请时from，验证时to
        pstmt->setInt(1, to); // from id
        pstmt->setInt(2, from);
        // 执行更新
        int rowAffected = pstmt->executeUpdate();
        if (rowAffected < 0) {
            return false;
        }
    }
    {
        // 插入认证方好友数据
        std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("INSERT IGNORE INTO friend(self_id, friend_id, back) "
            "VALUES (?, ?, ?) "
        ));
        //反过来的申请时from，验证时to
        pstmt->setInt(1, from); // from id
        pstmt->setInt(2, to);
        pstmt->setString(3, backname);
        // 执行更新
        int rowAffected = pstmt->executeUpdate();
        if (rowAffected < 0) {
            con->_con->rollback();
            return false;
        }

        //插入申请方好友数据
        std::unique_ptr<sql::PreparedStatement> pstmt2(con->_con->prepareStatement("INSERT IGNORE INTO friend(self_id, friend_id, back) "
            "VALUES (?, ?, ?) "
        ));
        //反过来的申请时from，验证时to
        pstmt2->setInt(1, to); // from id
        pstmt2->setInt(2, from);
        pstmt2->setString(3, "");
        // 执行更新
        int rowAffected2 = pstmt2->executeUpdate();
        if (rowAffected2 < 0) {
            con->_con->rollback();
            return false;
        }
    }
    return true;
}

bool MysqlDAO::GetUserThreads(int64_t userId, int64_t lastId, int pageSize,
    std::vector<std::shared_ptr<ChatThreadInfo>>& threads,
    bool& loadMore, int64_t& nextLastId)
{
    loadMore = false;
    nextLastId = lastId;
    threads.clear();

    auto con = pool_->getConnection();
    if (!con) return false;
    Defer defer([this, &con]() { pool_->returnConnection(std::move(con)); });

    try {
        // 【优化后的 SQL】：将 LIMIT 下推至子查询，极大地压榨临时表的体积
        std::string sql = R"(
            SELECT thread_id, type, user1_id, user2_id FROM (
                (SELECT thread_id, 'private' AS type, user1_id, user2_id 
                 FROM private_chat 
                 WHERE (user1_id = ? OR user2_id = ?) AND thread_id > ? 
                 ORDER BY thread_id ASC LIMIT ?)
                UNION ALL
                (SELECT thread_id, 'group' AS type, 0 AS user1_id, 0 AS user2_id 
                 FROM group_chat_member 
                 WHERE user_id = ? AND thread_id > ? 
                 ORDER BY thread_id ASC LIMIT ?)
            ) AS combined_threads
            ORDER BY thread_id ASC 
            LIMIT ?
        )";

        std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(sql));

        int limit_count = pageSize + 1; // 多查一条用于判断是否还有后续数据
        int idx = 1;
        pstmt->setInt64(idx++, userId);
        pstmt->setInt64(idx++, userId);
        pstmt->setInt64(idx++, lastId);
        pstmt->setInt(idx++, limit_count); // private 子查询限制

        pstmt->setInt64(idx++, userId);
        pstmt->setInt64(idx++, lastId);
        pstmt->setInt(idx++, limit_count); // group 子查询限制

        pstmt->setInt(idx++, limit_count); // 最终外层聚合限制

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

        while (res->next()) {
            auto cti = std::make_shared<ChatThreadInfo>();
            cti->_thread_id = res->getInt64("thread_id");
            cti->_type = res->getString("type");
            cti->_user1_id = res->getInt64("user1_id");
            cti->_user2_id = res->getInt64("user2_id");
            threads.push_back(cti);
        }

        if (threads.size() > pageSize) {
            loadMore = true;
            threads.pop_back(); // 剔除多取的那一条
        }

        if (!threads.empty()) {
            nextLastId = threads.back()->_thread_id;
        }
        return true;
    }
    catch (sql::SQLException& e) {
        std::cerr << "SQLException: " << e.what() << " (MySQL error code: " << e.getErrorCode() << ")" << std::endl;
        return false;
    }
}
