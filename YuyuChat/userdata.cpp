#include "userdata.h"


void UserInfo::AppendChatMsgs(std::vector<std::shared_ptr<TextChatData> > msgs)
{
    for(const auto & msg :msgs){
        _chat_msgs.push_back(msg);
    }
}
