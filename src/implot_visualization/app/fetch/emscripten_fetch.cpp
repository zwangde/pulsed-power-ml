#include <deserialize_json.h>
#include <emscripten.h>
#include <emscripten/fetch.h>
#include <emscripten_fetch.h>
#include <format>
#include <iostream>
#include <string.h>

template<typename T>
void Subscription<T>::downloadSucceeded(emscripten_fetch_t *fetch) {
    // The data is now available at fetch->data[0] through fetch->data[fetch->numBytes-1];
    this->acquisition.jsonString.assign(fetch->data, fetch->numBytes);

    emscripten_fetch_close(fetch); // Free data associated with the fetch.
    this->fetchSuccessful = true;
}

template<typename T>
void Subscription<T>::downloadFailed(emscripten_fetch_t *fetch) {
    printf("Downloading %s failed, HTTP failure status code: %d.\n", fetch->url, fetch->status);
    emscripten_fetch_close(fetch); // Also free data on failure.
    this->fetchFinished = true;
}

template<typename T>
void onDownloadSucceeded(emscripten_fetch_t *fetch) {
    Subscription<T> *sub = static_cast<Subscription<T> *>(fetch->userData);
    sub->downloadSucceeded(fetch);
}

template<typename T>
void onDownloadFailed(emscripten_fetch_t *fetch) {
    Subscription<T> *sub = static_cast<Subscription<T> *>(fetch->userData);
    sub->downloadFailed(fetch);
}

template<typename T>
Subscription<T>::Subscription(const std::string &_url, const std::vector<std::string> &_requestedSignals)
    : url(_url), requestedSignals(_requestedSignals) {
    for (std::string str : _requestedSignals) {
        this->url = this->url + str + ",";
    }
    if (!this->url.empty()) {
        this->url.pop_back();
    }

    T _acquisition(_requestedSignals);
    this->acquisition = _acquisition;

    if (url.find("channelNameFilter") != std::string::npos) {
        this->extendedUrl = this->url + "&lastRefTrigger=0";
    } else {
        this->extendedUrl = this->url;
    }
}

template<typename T>
void Subscription<T>::fetch() {
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "GET");
    // static const char *custom_headers[3] = { "X-OPENCMW-METHOD", "POLL", nullptr };
    // attr.requestHeaders = custom_headers;
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    attr.onsuccess  = onDownloadSucceeded<T>;
    attr.onerror    = onDownloadFailed<T>;
    attr.userData   = this;

    if (this->fetchFinished) {
        this->fetchFinished = false;
        emscripten_fetch(&attr, this->extendedUrl.c_str());
    }
    if (fetchSuccessful) {
        this->acquisition.deserialize();
        if (this->url.find("limiting_curve") == std::string::npos) {
            updateUrl();
        }
        this->fetchSuccessful = false;
        this->fetchFinished   = true;
    }
}

template<typename T>
void Subscription<T>::updateUrl() {
    this->extendedUrl = this->url + "&lastRefTrigger=" + std::to_string(acquisition.lastTimeStamp);
}

template class Subscription<Acquisition>;
template class Subscription<AcquisitionSpectra>;
