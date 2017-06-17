#pragma once

enum QCloudPktType {
    kAccepted,
    kHeartBeat,
    kSignup,
    kSignupOk,
    kSlaveSignup,
    kInformSlaveUserInfo, // master->slave, pkt: sid:4B | name_len | name | pwd:32B
    kInformSlaveUserInfoReply, // slave->master, pkt: sid:4B
    kInformClientSlaveInfo, // master->client, pkt: ip_len | ip | port:2B
    kUpdatePwd,
    kLogin,
    kCheckingMd5,

    kDownloadRequest,
    kDownloadReply, // state --> kDownloading
    kDownloadBlockRequest, // client request
    kDownloadBlockReply,
    kDownloadComplete, // client to srv, kDownloading --> kLogined

    kNoSlaveAvailable,
    kSlaveNotOnline,

    kUploadRequest,
    kUploadBlockRequest, // srv请求, pkt: rcvLen:8B
    kPeerUploading,
    kQuickUploadSucceed,
    kUploadBlock, // client被动推文件块

    kFileListRequest, // client->srv
    kFileListReply, // srv->client

    kDeleteFileRequest, // client->srv, pkt: nameLen: int32_t | name: string // "qiu/abc.mkv"

    kTooManyConnections,
    kDumplicatedName,
    kNameOrPwdErr,
    kNameTooShort,
    kNameTooLong,
    kPwdTooShort,
    kPwdTooLong,

    kNoSuchFile,
    kMd5IsSame,
    kMd5NotSame,
    kUploadErr,
};

enum QCloudLimit {
    kMaxPacketLen = 1 << 20,
    kMinPacketLen = 4,
    kMaxNameLen = 20,
    kMinNameLen = 1,
    kMaxPwdLen = 20,
    kMinPwdLen = 1,
};
