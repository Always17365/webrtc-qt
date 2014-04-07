/*
 * JniPeerConnection.cpp
 *
 *  Created on: 2014-4-1
 *      Author: lht
 */

#include "JniPeerConnection.h"
#include "talk/base/bind.h"
#include "JniUtil.h"
JniPeerConnection::JniPeerConnection() {
	jni_thread = talk_base::Thread::Current();
}

JniPeerConnection::~JniPeerConnection() {

}

bool JniPeerConnection::SendToPeer(const std::string& peer_id,
		const std::string& message) {
	return jni_thread->Invoke<bool>(
			talk_base::Bind(&JniPeerConnection::SendToPeer_jni, this,peer_id,message));

}

bool JniPeerConnection::SendToPeer_jni(const std::string& peer_id,
		const std::string& message) {
	return JniUtil::GetInstance()->JniSendToPeer(peer_id.c_str(),message.c_str());
}


