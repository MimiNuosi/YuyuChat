#pragma once
#include<boost/beast/http.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <memory>
#include <iostream>
#include "Singleton.h"
#include <functional>
#include <map>
#include <queue>
#include <atomic>
#include <unordered_map>
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include <cassert>


namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

#define HEAD_DATA_LEN 2
#define HEAD_ID_LEN 2
#define HEAD_TOTAL_LEN 4
#define MAX_LENGTH  1024*64
#define MAX_RECVQUE  10000
#define MAX_SENDQUE 1000
//4���߼�������
#define LOGIC_WORKER_COUNT 4
//4���ļ�������
#define FILE_WORKER_COUNT 4
//4�����ع�����
#define DOWN_LOAD_WORKER_COUNT	4
#define MAX_FILE_LEN 1024*32

enum ErrorCodes {
	Success = 0,

	// 1000 ~ 1099: ͨ�����Ȩ����
	Error_Json = 1001,
	RPCFailed = 1002,
	TokenInvalid = 1010,
	UidInvalid = 1011,

	// 1100 ~ 1199: ChatServer ר��ҵ�����
	CREATE_CHAT_FAILED = 1101,
	LOAD_CHAT_FAILED = 1102,

	// 1200 ~ 1299: ResourceServer ר���ļ�����
	FileNotExists = 1201,
	FileSaveRedisFailed = 1202,
	CreateFilePathFailed = 1203,
	FileWritePermissionFailed = 1204,
	FileReadPermissionFailed = 1205,
	FileSeqInvalid = 1206,
	FileOffsetInvalid = 1207,
	FileReadFailed = 1208,
	RedisReadErr = 1209,
	ServerIpErr = 1210,
	MsgIdErr = 1211,
};

enum MSG_IDS {
    // ================= 1001 ~ 1030: �������û����� (ChatServer) =================
    MSG_CHAT_LOGIN = 1005, // �û���¼
    MSG_CHAT_LOGIN_RSP = 1006, // �û���¼�ذ�
    ID_SEARCH_USER_REQ = 1007,
    ID_SEARCH_USER_RSP = 1008,
    ID_ADD_FRIEND_REQ = 1009,
    ID_ADD_FRIEND_RSP = 1010,
    ID_NOTIFY_ADD_FRIEND_REQ = 1011,
    ID_AUTH_FRIEND_REQ = 1013,
    ID_AUTH_FRIEND_RSP = 1014,
    ID_NOTIFY_AUTH_FRIEND_REQ = 1015,
    ID_TEXT_CHAT_MSG_REQ = 1017,
    ID_TEXT_CHAT_MSG_RSP = 1018,
    ID_NOTIFY_TEXT_CHAT_MSG_REQ = 1019,
    ID_NOTIFY_OFF_LINE_REQ = 1021,
    ID_HEART_BEAT_REQ = 1023,
    ID_HEARTBEAT_RSP = 1024,
    ID_LOAD_CHAT_THREAD_REQ = 1025,
    ID_LOAD_CHAT_THREAD_RSP = 1026,
    ID_CREATE_PRIVATE_CHAT_REQ = 1027,
    ID_CREATE_PRIVATE_CHAT_RSP = 1028,
    ID_LOAD_CHAT_MSG_REQ = 1029,
    ID_LOAD_CHAT_MSG_RSP = 1030,

    // ================= 1031 ~ 1060: �ļ����ý����Դ���� (ResourceServer) =================
    ID_UPLOAD_HEAD_ICON_REQ = 1031, // �ϴ�ͷ������
    ID_UPLOAD_HEAD_ICON_RSP = 1032, // �ϴ�ͷ��ظ�
    ID_DOWN_LOAD_FILE_REQ = 1033, // �����ļ�����
    ID_DOWN_LOAD_FILE_RSP = 1034, // �����ļ��ظ�
    ID_IMG_CHAT_MSG_REQ = 1035,
    ID_IMG_CHAT_MSG_RSP = 1036,
    ID_IMG_CHAT_UPLOAD_REQ = 1037, // �ϴ�����ͼƬ��Դ
    ID_IMG_CHAT_UPLOAD_RSP = 1038, // �ϴ�����ͼƬ�ظ�
    ID_NOTIFY_IMG_CHAT_MSG_REQ = 1039, // ֪ͨ�û�ͼƬ��Ϣ
    ID_FILE_INFO_SYNC_REQ = 1041, // �ļ���Ϣͬ������
    ID_FILE_INFO_SYNC_RSP = 1042, // �ļ���Ϣͬ���ظ�
    ID_IMG_CHAT_CONTINUE_UPLOAD_REQ = 1043, // ��������ͼƬ����
    ID_IMG_CHAT_CONTINUE_UPLOAD_RSP = 1044, // ��������ͼƬ�ظ�
    ID_IMG_CHAT_DOWN_INFO_SYNC_REQ = 1045, // ��ȡ����ͼƬ����ͬ����Ϣ
    ID_IMG_CHAT_DOWN_INFO_SYNC_RSP = 1046, // ��ȡ����ͼƬ����ͬ����Ϣ�ظ�
    ID_IMG_CHAT_DOWN_REQ = 1047, // ����ͼƬ��������
    ID_IMG_CHAT_DOWN_RSP = 1048, // ����ͼƬ���ػظ�

    ID_TEST_MSG_REQ = 1051,
    ID_TEST_MSG_RSP = 1052,
    ID_UPLOAD_FILE_REQ = 1053,
    ID_UPLOAD_FILE_RSP = 1054,
    ID_SYNC_FILE_REQ = 1055,
    ID_SYNC_FILE_RSP = 1056,
};

enum MsgStatus {
    UN_READ = 0,  //对方未读
    SEND_FAILED = 1,  //发送失败
    READED = 2,  //对方已读
    UN_UPLOAD = 3 //未上传完成
};

class Defer {
public:
	Defer(std::function<void()> func) : _func(func) {}
	~Defer() { _func(); }
private:
	std::function<void()> _func;
};

constexpr const char* CODEPREFIX = "code_";
constexpr const char* NAME_INFO = "nameinfo_";
constexpr const char* USERTOKENPREFIX = "utoken_";
constexpr const char* USERIPPREFIX = "uip_";
constexpr const char* IPCOUNTPREFIX = "ipcount_";
constexpr const char* USER_BASE_INFO = "ubaseinfo_";
constexpr const char* LOGIN_COUNT = "logincount";
constexpr const char* LOCK_PREFIX = "lock_";
constexpr const char* USER_SESSION_PREFIX = "usession_";
//�ֲ�ʽ���ĳ���ʱ��
constexpr const int LOCK_TIME_OUT = 10;
//�ֲ�ʽ��������ʱ��
constexpr const int ACQUIRE_TIME_OUT = 5;
//������ֵ���룩
constexpr const int HEART_BEAT_THRESHOLD = 60;