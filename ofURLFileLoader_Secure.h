#pragma once

#include "ofURLFileLoader.h"
class ofURLFileLoader_Secure : public ofURLFileLoader {
public:
    ofHttpResponse handleRequest(const ofHttpRequest & request) {
        // debug log option
        //curl_easy_setopt(curl.get(), CURLOPT_VERBOSE, 1L);

        curl_slist *headers = nullptr;
        curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYPEER, 0);

        // unsecure connect. This is default of ofURLFileLoader
        //curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYHOST, 0);
        
        // secure connect
        curl_easy_setopt(curl.get(), CURLOPT_SSL_VERIFYHOST, 1L);

        curl_easy_setopt(curl.get(), CURLOPT_URL, request.url.c_str());

        // always follow redirections
        curl_easy_setopt(curl.get(), CURLOPT_FOLLOWLOCATION, 1L);

        // Set content type and any other header
        if(request.contentType!=""){
            headers = curl_slist_append(headers, ("Content-Type: " + request.contentType).c_str());
        }
        for(map<string,string>::const_iterator it = request.headers.cbegin(); it!=request.headers.cend(); it++){
            headers = curl_slist_append(headers, (it->first + ": " +it->second).c_str());
        }

        curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, headers);

        std::string body = request.body;

        // set body if there's any
        if(request.body!=""){
    //        curl_easy_setopt(curl.get(), CURLOPT_UPLOAD, 1L); // Tis does PUT instead of POST
            curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDSIZE, request.body.size());
            curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, nullptr);
            //curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, request.body.c_str());
            curl_easy_setopt(curl.get(), CURLOPT_READFUNCTION, readBody_cb);
            curl_easy_setopt(curl.get(), CURLOPT_READDATA, &body);
        }else{
    //        curl_easy_setopt(curl.get(), CURLOPT_UPLOAD, 0L);
            curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDSIZE, 0);
            //curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, nullptr);
            curl_easy_setopt(curl.get(), CURLOPT_READFUNCTION, nullptr);
            curl_easy_setopt(curl.get(), CURLOPT_READDATA, nullptr);
        }
        if(request.method == ofHttpRequest::GET){
            curl_easy_setopt(curl.get(), CURLOPT_HTTPGET, 1);
            curl_easy_setopt(curl.get(), CURLOPT_POST, 0);
        }else{
            curl_easy_setopt(curl.get(), CURLOPT_POST, 1);
            curl_easy_setopt(curl.get(), CURLOPT_HTTPGET, 0);
        }

        if(request.timeoutSeconds>0){
            curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, request.timeoutSeconds);
        }

        // start request and receive response
        ofHttpResponse response(request, 0, "");
        CURLcode err = CURLE_OK;
        if(request.saveTo){
            ofFile saveTo(request.name, ofFile::WriteOnly, true);
            curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &saveTo);
            curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, saveToFile_cb);
            err = curl_easy_perform(curl.get());
        }else{
            curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &response);
            curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, saveToMemory_cb);
            err = curl_easy_perform(curl.get());
        }
        if(err==CURLE_OK){
            long http_code = 0;
            curl_easy_getinfo (curl.get(), CURLINFO_RESPONSE_CODE, &http_code);
            response.status = http_code;
        }else{
            response.error = curl_easy_strerror(err);
            response.status = -1;
        }

        if(headers){
            curl_slist_free_all(headers);
        }

        return response;
    }
}
