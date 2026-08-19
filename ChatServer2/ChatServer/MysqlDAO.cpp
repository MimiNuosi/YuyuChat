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

		// 3 插入好友数据到 friend 表
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
            pstmt2->setString(3, reverse_back);
            // 执行更新
            int rowAffected2 = pstmt2->executeUpdate();
            if (rowAffected2 < 0) {
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

    auto& connect = con->_con;
    Defer defer([this, &con, &connect]() {
        if (connect && !connect->isClosed()) {
            connect->setAutoCommit(true);
        }
        pool_->returnConnection(std::move(con));
        });

    try
    {
        connect->setAutoCommit(false);

        int64_t uid1 = std::min(user1Id, user2Id);
        int64_t uid2 = std::max(user1Id, user2Id);

        std::unique_ptr<sql::PreparedStatement> pstmt(connect->prepareStatement(
            "SELECT thread_id FROM private_chat WHERE user1_id = ? AND user2_id = ? FOR UPDATE;"));
        pstmt->setInt64(1, uid1);
        pstmt->setInt64(2, uid2);

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        if (res->next()) {
            threadId = res->getInt64(1);
            connect->commit();
            return true;
        }

        std::unique_ptr<sql::PreparedStatement> insertStmt(connect->prepareStatement(
            "INSERT INTO chat_thread (type, created_at) VALUES (?, NOW());"));
        insertStmt->setString(1, "private");
        insertStmt->executeUpdate();

        std::unique_ptr<sql::PreparedStatement> selectStmt(connect->prepareStatement(
            "SELECT LAST_INSERT_ID();"));
        std::unique_ptr<sql::ResultSet> res_last_id(selectStmt->executeQuery());
        if (res_last_id->next()) {
            threadId = res_last_id->getInt64(1);
        }
        else {
            connect->rollback();
            return false;
        }

        std::unique_ptr<sql::PreparedStatement> insertPrivateStmt(connect->prepareStatement(
            "INSERT INTO private_chat (thread_id, user1_id, user2_id, created_at) VALUES (?, ?, ?, NOW());"));
        insertPrivateStmt->setInt64(1, threadId);
        insertPrivateStmt->setInt64(2, uid1);
        insertPrivateStmt->setInt64(3, uid2);
        insertPrivateStmt->executeUpdate();

        connect->commit();
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "SQLException: " << e.what() << std::endl;
        try {
            connect->rollback();
        }
        catch (...) {}
        return false;
    }
}