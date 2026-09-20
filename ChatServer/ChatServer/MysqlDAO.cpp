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

bool MysqlDAO::AddFriend(const int& from, const int& to, const std::string& desc, const std::string& back_name) {
    auto con = pool_->getConnection();
    if (con == nullptr) {
        return false;
    }

    Defer defer([this, &con]() {
        pool_->returnConnection(std::move(con));
        });

    try {
        std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("INSERT INTO friend_apply (from_uid, to_uid, descs, back_name) "
				"values (?,?,?,?) "
            "ON DUPLICATE KEY UPDATE from_uid = from_uid, to_uid = to_uid, descs = ?, back_name = ?"));
        pstmt->setInt(1, from);
        pstmt->setInt(2, to);
        pstmt->setString(3, desc);
        pstmt->setString(4, back_name);
        pstmt->setString(5, desc);
        pstmt->setString(6, back_name);
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

bool MysqlDAO::AuthFriend(const int& from, const int& to, std::string backname,
    std::vector<std::shared_ptr<AddFriendMsg>>& chat_datas) {
    auto con = pool_->getConnection();
    if (con == nullptr) {
        return false;
    }

    Defer defer([this, &con]() {
        pool_->returnConnection(std::move(con));
        });

    try {
		con->_con->setAutoCommit(false); // 开启事务
        std::string reverse_back;
        std::string apply_desc;

		// 1 查询 friend_apply 表，获取对应的申请记录
        {
			std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement
            ("SELECT back_name, descs FROM friend_apply WHERE from_uid = ? AND to_uid = ?"));
            pstmt->setInt(1,to);
			pstmt->setInt(2,from);
            std::unique_ptr<sql::ResultSet> rsSel(pstmt->executeQuery());

            if (rsSel->next()) {
                reverse_back = rsSel->getString("back_name");
                apply_desc = rsSel->getString("descs");
            }
            else {
                // 没有对应的申请记录，直接 rollback 并返回失败
                con->_con->rollback();
                return false;
            }
        }
		// 2 更新 friend_apply 表，将 status 更新为 1
        {
            std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("UPDATE friend_apply SET status = 1 "
                "WHERE from_uid = ? AND to_uid = ?"));
            //反过来的申请时from，验证时to
            pstmt->setInt(1, to); // from id
            pstmt->setInt(2, from);
            // 执行更新
            int rowAffected = pstmt->executeUpdate();
            if (rowAffected < 0) {
				con->_con->rollback();
                return false;
            }
        }

        // 3. 按照全局确定的 uid 大小升序插入 friend 记录，消除交叉加锁死锁
        {
            int uid1 = std::min(from, to);
            int uid2 = std::max(from, to);

            std::string back1 = (from == uid1) ? backname : reverse_back;
            std::string back2 = (from == uid2) ? backname : reverse_back;

            std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
                "INSERT IGNORE INTO friend(self_id, friend_id, back) VALUES (?, ?, ?)"
            ));

            // 第一次插入：较小 uid 占 self_id
            pstmt->setInt(1, uid1);
            pstmt->setInt(2, uid2);
            pstmt->setString(3, back1);
            if (pstmt->executeUpdate() < 0) {
                con->_con->rollback();
                return false;
            }

            // 第二次插入：较大 uid 占 self_id
            pstmt->setInt(1, uid2);
            pstmt->setInt(2, uid1);
            pstmt->setString(3, back2);
            if (pstmt->executeUpdate() < 0) {
                con->_con->rollback();
                return false;
            }
        }

		//4 创建数据到 chat_thread 表
        int64_t threadId = 0;
        {
            std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
				"INSERT INTO chat_thread (type, created_at) VALUES (?, NOW());"));
            pstmt->setString(1, "private");
            pstmt->executeUpdate();
            std::unique_ptr<sql::PreparedStatement> pstmtLastId(con->_con->prepareStatement(
                "SELECT LAST_INSERT_ID();"));
            std::unique_ptr<sql::ResultSet> res_last_id(pstmtLastId->executeQuery());
            if (res_last_id->next()) {
                threadId = res_last_id->getInt64(1);
            }
            else {
                con->_con->rollback();
                return false;
			}
        }

        //5 插入数据到 private_chat 表
        {
            std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
                "INSERT INTO private_chat (thread_id, user1_id, user2_id) VALUES (?, ?, ?);"));
            pstmt->setInt64(1, threadId);
            pstmt->setInt64(2, std::min(from, to));
            pstmt->setInt64(3, std::max(from, to));
            int rowAffected = pstmt->executeUpdate();
            if (rowAffected < 0) {
                con->_con->rollback();
                return false;
            }
		}

        //6 插入初始消息到chat_message表
        if (!apply_desc.empty())
        {
            std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
                "INSERT INTO chat_message (thread_id, sender_id, recv_id, content, created_at, updated_at, status) "
                "VALUES (?, ?, ?, ?, NOW(), NOW(), ?);"));
            pstmt->setInt64(1, threadId);
            pstmt->setInt64(2, to);
            pstmt->setInt64(3, from);
            pstmt->setString(4, apply_desc);
            pstmt->setInt(5, 0);
            int rowAffected = pstmt->executeUpdate();
            if (rowAffected < 0) {
                con->_con->rollback();
                return false;
            }
            std::unique_ptr<sql::Statement> stmt(con->_con->createStatement());
            std::unique_ptr<sql::ResultSet> rs(
                stmt->executeQuery("SELECT LAST_INSERT_ID()")
            );
            if (rs->next()) {
                auto messageId = rs->getInt64(1);
                auto tx_data = std::make_shared<AddFriendMsg>();
                tx_data->set_sender_id(to);
                tx_data->set_msg_id(messageId);
                tx_data->set_msgcontent(apply_desc);
                tx_data->set_thread_id(threadId);
                tx_data->set_unique_id("");
                std::cout << "addfriend insert message success" << std::endl;
                chat_datas.push_back(tx_data);
            }
            else {
                return false;
            }
		}
        {
            std::unique_ptr<sql::PreparedStatement> msgStmt(con->_con->prepareStatement(
                "INSERT INTO chat_message(thread_id, sender_id, recv_id, content, created_at, updated_at, status) VALUES (?, ?, ?, ?,NOW(),NOW(),?)"
            ));

            msgStmt->setInt64(1, threadId);
            msgStmt->setInt(2, from);
            msgStmt->setInt(3, to);
            msgStmt->setString(4, "我们已经是好友了，现在开始聊天吧!");

            msgStmt->setInt(5, 0);

            if (msgStmt->executeUpdate() < 0) { return false; }

            std::unique_ptr<sql::Statement> stmt(con->_con->createStatement());
            std::unique_ptr<sql::ResultSet> rs(
                stmt->executeQuery("SELECT LAST_INSERT_ID()")
            );
            if (rs->next()) {
                auto messageId = rs->getInt64(1);
                auto tx_data = std::make_shared<AddFriendMsg>();
                tx_data->set_sender_id(from);
                tx_data->set_msg_id(messageId);
                tx_data->set_msgcontent("我们已经是好友了，现在开始聊天吧!");
                tx_data->set_thread_id(threadId);
                tx_data->set_unique_id("");
                chat_datas.push_back(tx_data);
            }
            else {
                return false;
            }
        }
		con->_con->commit(); // 提交事务
        return true;
    }
    catch(sql::SQLException& e){
        if (con&&con->_con) {
			con->_con->rollback(); // 回滚事务
        }
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return false;
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

bool MysqlDAO::CreatePrivateThread(int64_t user1Id, int64_t user2Id, int64_t& threadId)
{
    auto con = pool_->getConnection();
    if (!con) return false;
    Defer defer([this, &con]() { pool_->returnConnection(std::move(con)); });
    auto& conn = con->_con;

    int uid1 = std::min(user1Id, user2Id);
    int uid2 = std::max(user1Id, user2Id);

    // 1. 极速路径：普通快照读（无锁、无 Gap Lock、不阻塞），99% 的老会话在此处 1 次 RTT 直接返回
    try {
        std::unique_ptr<sql::PreparedStatement> pstmt_query(
            conn->prepareStatement("SELECT thread_id FROM private_chat WHERE user1_id = ? AND user2_id = ?")
        );
        pstmt_query->setInt64(1, uid1);
        pstmt_query->setInt64(2, uid2);
        std::unique_ptr<sql::ResultSet> res(pstmt_query->executeQuery());
        if (res->next()) {
            threadId = res->getInt("thread_id");
            return true;
        }
    }
    catch (sql::SQLException& e) {
        std::cerr << "Query check failed: " << e.what() << std::endl;
        return false;
    }

    // 2. 慢路径：首次建立私聊，开启事务插入
    try {
        conn->setAutoCommit(false);

        // a. 创建公共线程
        std::unique_ptr<sql::PreparedStatement> pstmt_thread(
            conn->prepareStatement("INSERT INTO chat_thread (type, created_at) VALUES ('private', NOW())")
        );
        pstmt_thread->executeUpdate();

        std::unique_ptr<sql::Statement> key_stmt(conn->createStatement());
        std::unique_ptr<sql::ResultSet> res_id(key_stmt->executeQuery("SELECT LAST_INSERT_ID()"));
        if (!res_id->next()) {
            conn->rollback();
            conn->setAutoCommit(true);
            return false;
        }
        threadId = res_id->getInt(1);

        // b. 插入关联表（依赖唯一索引 uk_user_pair）
        std::unique_ptr<sql::PreparedStatement> pstmt_insert(
            conn->prepareStatement("INSERT INTO private_chat (thread_id, user1_id, user2_id, created_at) VALUES (?, ?, ?, NOW())")
        );
        pstmt_insert->setInt64(1, threadId);
        pstmt_insert->setInt64(2, uid1);
        pstmt_insert->setInt64(3, uid2);
        pstmt_insert->executeUpdate();

        conn->commit();
        conn->setAutoCommit(true); //归还前恢复连接状态
        return true;
    }
    catch (sql::SQLException& e) {
        conn->rollback();
        conn->setAutoCommit(true); // 发生异常必须先回滚并重置状态

        // 3. 冲突兜底：极端并发下两个用户同时点发起，输掉的事务捕获 1062，查出赢家创建的 thread_id
        if (e.getErrorCode() == 1062) {
            try {
                std::unique_ptr<sql::PreparedStatement> pstmt_retry(
                    conn->prepareStatement("SELECT thread_id FROM private_chat WHERE user1_id = ? AND user2_id = ?")
                );
                pstmt_retry->setInt64(1, uid1);
                pstmt_retry->setInt64(2, uid2);
                std::unique_ptr<sql::ResultSet> res(pstmt_retry->executeQuery());
                if (res->next()) {
                    threadId = res->getInt("thread_id");
                    return true;
                }
            }
            catch (sql::SQLException& e2) {
                std::cerr << "Query retry failed: " << e2.what() << std::endl;
            }
        }
        std::cerr << "CreatePrivateChat failed: " << e.what() << std::endl;
        return false;
    }
}

std::shared_ptr<PageResult> MysqlDAO::LoadChatMessages(int64_t threadId, int64_t lastId, int pageSize)
{
	auto con = pool_->getConnection();
	if (!con) return nullptr;
	Defer defer([this, &con]() { pool_->returnConnection(std::move(con)); });

	auto& conn = con->_con;

    try {
		auto pageResult = std::make_shared<PageResult>();
		pageResult->loadMore = false;
		pageResult->nextLastId = lastId;

        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(R"(
        SELECT message_id, thread_id, unique_id, msg_type, sender_id, recv_id, content,
               created_at, updated_at, status
        FROM chat_message
        WHERE thread_id = ?
          AND message_id > ?
        ORDER BY message_id ASC
        LIMIT ?
        )"));

		auto limit_count = pageSize + 1; // 多查一条用于判断是否还有后续数据

		pstmt->setInt64(1, threadId);
		pstmt->setInt64(2, lastId);
		pstmt->setInt(3, limit_count);

		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

        while (res->next())
        {
            ChatMessage msg;
            msg.msg_type = res->getInt("msg_type");
            msg.unique_id = res->getString("unique_id");
            msg.message_id = res->getUInt64("message_id");
            msg.thread_id = res->getUInt64("thread_id");
            msg.sender_id = res->getUInt64("sender_id");
            msg.recv_id = res->getUInt64("recv_id");
            msg.content = res->getString("content");
            msg.chat_time = res->getString("created_at");
            msg.status = res->getInt("status");
			pageResult->messages.push_back(std::make_shared<ChatMessage>(msg));
        }

        if (pageResult->messages.size() > pageSize) {
            pageResult->loadMore = true;
            pageResult->messages.pop_back(); // 剔除多取的那一条
		}
		return pageResult;  
    }
    catch (const std::exception& e) {
        std::cerr << "SQLException: " << e.what() << std::endl;
        return nullptr;
	}
	return nullptr;
}

bool MysqlDAO::AddChatMessage(std::vector<std::shared_ptr<ChatMessage>>& chat_datas)
{
    auto con = pool_->getConnection();
    if (!con) {
        return false;
    }

    Defer defer([this, &con]() {
        pool_->returnConnection(std::move(con));
        });
    auto& conn = con->_con;

    try
    {
        //关闭自动提交，以手动管理事务
        conn->setAutoCommit(false);
        auto pstmt = std::unique_ptr<sql::PreparedStatement>(
            conn->prepareStatement(
                "INSERT INTO chat_message "
                "(thread_id, unique_id, msg_type, sender_id, recv_id, content, created_at, updated_at, status) "
                "VALUES (?, ?, ? , ?, ?, ?, NOW(), NOW(), ?)"
            )
        );

        for (auto& msg : chat_datas) {
            pstmt->setUInt64(1, msg->thread_id);
            pstmt->setString(2, msg->unique_id);
            pstmt->setUInt64(3, msg->msg_type);
            pstmt->setUInt64(4, msg->sender_id);
            pstmt->setUInt64(5, msg->recv_id);
            pstmt->setString(6, msg->content);
            pstmt->setInt(7, msg->status); // 对应第 5 个占位符
            pstmt->executeUpdate();

            //  取 LAST_INSERT_ID()
            std::unique_ptr<sql::Statement> keyStmt(
                conn->createStatement()
            );
            std::unique_ptr<sql::ResultSet> rs(
                keyStmt->executeQuery("SELECT LAST_INSERT_ID()")
            );
            if (rs->next()) {
                msg->message_id = rs->getUInt64(1);
            }
            else {
                continue;
            }
        }

        conn->commit();
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "SQLException: " << e.what() << std::endl;
    }
    return false;
}
