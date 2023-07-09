#include "includes.h"
#include "network/HttpClient.h"


static extension::CCHttpClient* (__thiscall* CCHttpClient_c)(extension::CCHttpClient* self);

static extension::CCHttpClient* __fastcall CCHttpClient_c_H(extension::CCHttpClient* self, void*) {

    auto ret = CCHttpClient_c(self);

    return ret;
}

static void(__thiscall* CCHttpClient_destroyInstance)(extension::CCHttpClient* self);

static void __fastcall CCHttpClient_destroyInstance_H(extension::CCHttpClient* self, void*) {

    network::HttpClient::destroyInstance();

    CCHttpClient_destroyInstance(self);
}

static void(__thiscall* CCHttpClient_dispatchResponseCallbacks)(extension::CCHttpClient* self, float);

static void __fastcall CCHttpClient_dispatchResponseCallbacks_H(extension::CCHttpClient* self, void*, float dt) {
    //voided
}

//don't hook getInstance, as this instance will have necessary hooks anyways

//lol misspelled in cocos L + ratio
static bool(__thiscall* CCHttpClient_lazyInitThreadSemphore)(extension::CCHttpClient* self);

static bool __fastcall CCHttpClient_lazyInitThreadSemphore_H(extension::CCHttpClient* self, void*) {
    //voided
    return true;
}

static void(__thiscall* CCHttpClient_send)(extension::CCHttpClient* self, extension::CCHttpRequest*);

static void __fastcall CCHttpClient_send_H(extension::CCHttpClient* self, void*, extension::CCHttpRequest* request) {
    
    network::HttpRequest* newRequest = new network::HttpRequest();

    request->retain();

    network::HttpRequest::Type type;

    switch (request->getRequestType())
    {
    case extension::CCHttpRequest::kHttpGet:
        type = network::HttpRequest::Type::GET;
        break;
    case extension::CCHttpRequest::kHttpPost:
        type = network::HttpRequest::Type::POST;
        break;
    case extension::CCHttpRequest::kHttpPut:
        type = network::HttpRequest::Type::PUT;
        break;
    case extension::CCHttpRequest::kHttpDelete:
        type = network::HttpRequest::Type::DELETE;
        break;
    case extension::CCHttpRequest::kHttpUnkown:
        type = network::HttpRequest::Type::UNKNOWN;
        break;
    }

    newRequest->setRequestType(type);
    newRequest->setUrl(request->getUrl());
    newRequest->setRequestData(request->getRequestData(), request->getRequestDataSize());
    newRequest->setTag(request->getTag());
    newRequest->setUserData(request->getUserData());
    newRequest->setHeaders(request->getHeaders());

    newRequest->setResponseCallback([=](network::HttpClient* client, network::HttpResponse* response){
        extension::SEL_HttpResponse pSelector = request->getSelector();
        CCObject* pTarget = request->getTarget();
        
        extension::CCHttpClient* oldClient = extension::CCHttpClient::getInstance();
        extension::CCHttpResponse* oldResponse = new extension::CCHttpResponse(request);
        oldResponse->setSucceed(response->isSucceed());
        std::vector<char>* charData = new std::vector<char>();
        for (int i = 0; i < response->getResponseData()->size(); i++) {
            charData->push_back(response->getResponseData()->at(i));
        }
        oldResponse->setResponseData(charData);
        delete charData;
        oldResponse->setResponseCode(response->getResponseCode());

        if (pTarget && pSelector) {
            (pTarget->*pSelector)(oldClient, oldResponse);
        }
        oldClient->release();
        oldResponse->release();
    });
    network::HttpClient::getInstance()->send(newRequest);

    newRequest->release();

}


static void setupHooks() {

    CreateHook(ExtAddr("??0CCHttpClient@extension@cocos2d@@AAE@XZ"), CCHttpClient_c);
    CreateHook(ExtAddr("?destroyInstance@CCHttpClient@extension@cocos2d@@SAXXZ"), CCHttpClient_destroyInstance);
    CreateHook(ExtAddr("?dispatchResponseCallbacks@CCHttpClient@extension@cocos2d@@AAEXM@Z"), CCHttpClient_dispatchResponseCallbacks);
    CreateHook(ExtAddr("?lazyInitThreadSemphore@CCHttpClient@extension@cocos2d@@AAE_NXZ"), CCHttpClient_lazyInitThreadSemphore);
    CreateHook(ExtAddr("?send@CCHttpClient@extension@cocos2d@@QAEXPAVCCHttpRequest@23@@Z"), CCHttpClient_send);


}

