/*
 * libjingle
 * Copyright 2012, Google Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <iostream>  // NOLINT

#include "talk/base/asyncudpsocket.h"
#include "talk/base/optionsfile.h"
#include "talk/base/thread.h"
#include "talk/base/stringencode.h"
#include "talk/p2p/base/basicpacketsocketfactory.h"
#include "turnserver.h"

#include "asyndealer.h"
#include "talk/base/json.h"

static const char kSoftware[] = "kaer TurnServer";

class TurnFileAuth : public cricket::TurnAuthInterface {
public:
    explicit TurnFileAuth(const std::string& path) : file_(path) {
        if(!file_.Load()){
            std::cerr<<"file load error "<<path<<std::endl;
        }
    }
    virtual bool GetKey(const std::string& username, const std::string& realm,
                        std::string* key) {
        // File is stored as lines of <username>=<HA1>.
        // Generate HA1 via "echo -n "<username>:<realm>:<password>" | md5sum"
        std::string hex;
        bool ret = file_.GetStringValue(username, &hex);
        if (ret) {
            char buf[32];
            size_t len = talk_base::hex_decode(buf, sizeof(buf), hex);
            *key = std::string(buf, len);
        }else{
            std::cerr<<"Get name value error"<<std::endl;
        }
        return ret;
    }
private:
    talk_base::OptionsFile file_;
};


class TurnDealerAuth : public cricket::TurnAuthInterface {
public:

    explicit TurnDealerAuth(const std::string router):
        backStage("Backstage"),alarmStage("Alarmstage")
    {
        dealer = new AsynDealer();
        dealer->initialize("",router);

    }
    virtual bool GetKey(const std::string& username, const std::string& realm,
                        std::string* key) {

        Json::StyledWriter writer;
        Json::Value jmessage;
        jmessage["type"] = "Turn_GetPwd";
        jmessage["UserName"] = username;
        std::string msg = writer.write(jmessage);

        std::string recvMsg;

        if(!dealer->SendRecv("Backstage",msg,&recvMsg,3000)){
            LOG(INFO)<<"receive msg from backstage error";
            return false;
        }
        //{"Pwd": "067AE6D247D5BE64D341EABF0E7787A5","Result": 0,"type": "Turn_GetPwd"}
        Json::Reader reader;
        if (!reader.parse(recvMsg, jmessage)) {
            LOG(WARNING) << "Received unknown message. ";
            return false;
        }
        std::string pwdhex;
        if(!GetStringFromJsonObject(jmessage,"Pwd",&pwdhex)){
            LOG(INFO)<<"get pwd form receive msg error "<<recvMsg;
            return false;
        }
        LOG(INFO)<<"get user"<<username<<" pwd is "<<pwdhex;
        char buf[32];
        size_t len = talk_base::hex_decode(buf, sizeof(buf), pwdhex);
        *key = std::string(buf, len);
        return true;
    }

    virtual void ReportInfo(const std::string &username,int totalData,
                            const std::string &startTime,const std::string &endTime){
        LOG(LERROR)<<"ReportInfo---uname:"<<username<<"; total KB:"<<totalData<<
                   " ; start time :"<<startTime<<"; end time:"<<endTime;
        Json::StyledWriter writer;
        Json::Value jmessage;
        jmessage["type"] = "Turn_flowRecord";
        jmessage["UserName"] = username;
        jmessage["BeginTime"] = startTime;
        jmessage["EndTime"] = endTime;
        jmessage["flowTotal"] = totalData;//KB
        std::string msg = writer.write(jmessage);
        dealer->send(alarmStage,msg);
    }

    std::string realm;

private:
    std::string backStage;
    std::string alarmStage;
    AsynDealer * dealer;
};

int main(int argc, char **argv)
{
    if (argc != 5) {
        std::cerr << "usage: turnserver int-addr ext-ip realm router"
                  << std::endl;
        return 1;
    }

    talk_base::LogMessage::ConfigureLogging("tstamp thread error debug","");


    talk_base::SocketAddress int_addr;
    if (!int_addr.FromString(argv[1])) {
        std::cerr << "Unable to parse IP address: " << argv[1] << std::endl;
        return 1;
    }

    talk_base::IPAddress ext_addr;
    if (!IPFromString(argv[2], &ext_addr)) {
        std::cerr << "Unable to parse IP address: " << argv[2] << std::endl;
        return 1;
    }

    talk_base::Thread* main = talk_base::Thread::Current();
    talk_base::AsyncUDPSocket* int_socket =
            talk_base::AsyncUDPSocket::Create(main->socketserver(), int_addr);
    if (!int_socket) {
        std::cerr << "Failed to create a UDP socket bound at"
                  << int_addr.ToString() << std::endl;
        return 1;
    }



    TurnDealerAuth auth(argv[4]);
//    std::string key;
//    auth.GetKey("lht","kaer",&key);
//    auth.ReportInfo("lht",2222,GetCurrentDatetime("%F %T"),GetCurrentDatetime("%F %T"));
    cricket::TurnServer server(main);
    server.set_realm(argv[3]);
    server.set_software(kSoftware);
    server.set_auth_hook(&auth);
    server.AddInternalSocket(int_socket, cricket::PROTO_UDP);
    server.SetExternalSocketFactory(new talk_base::BasicPacketSocketFactory(),
                                    talk_base::SocketAddress(ext_addr, 0));

    std::cout << "Listening internally at " << int_addr.ToString() << std::endl;

    main->Run();
    return 0;
}