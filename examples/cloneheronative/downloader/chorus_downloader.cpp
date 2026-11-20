#include "chorus_downloader.hpp"
#include "song_package.hpp"
#include "http_get.hpp"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <pthread.h>
#include <string>
#include <vector>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#if !defined(PS5_HOST_REFERENCE)
extern "C" int sceNetInit(void);
extern "C" int sceNetPoolCreate(const char* name, int size, int flags);
extern "C" int sceSslInit(size_t poolSize);
extern "C" int sceHttp2Init(int netPoolId, int sslContextId, size_t poolSize, int flags);
extern "C" int sceHttp2CreateTemplate(int contextId, const char* userAgent, int httpVersion, int flags);
extern "C" int sceHttp2CreateRequestWithURL(int templateId, const char* method, const char* url, uint64_t contentLength);
extern "C" int sceHttp2AddRequestHeader(int requestId, const char* name, const char* value, unsigned int mode);
extern "C" int sceHttp2SetConnectTimeOut(int requestId, uint64_t timeout);
extern "C" int sceHttp2SetRecvTimeOut(int requestId, uint64_t timeout);
extern "C" int sceHttp2SetSendTimeOut(int requestId, uint64_t timeout);
extern "C" int sceHttp2SendRequest(int requestId, const void* body, size_t bodySize);
extern "C" int sceHttp2AbortRequest(int requestId);
extern "C" int sceHttp2GetStatusCode(int requestId, int* statusCode);
extern "C" int sceHttp2ReadData(int requestId, void* buffer, size_t size);
extern "C" int sceHttp2DeleteRequest(int requestId);
extern "C" int sceHttp2DeleteTemplate(int templateId);
extern "C" int sceHttp2SetAutoRedirect(int id, int enabled);
#endif

#if !defined(PS5_HOST_REFERENCE)
#define JSON_NOEXCEPTION 1
#include "json.hpp"
#endif

namespace chorus {

struct Downloader::HttpTransfer {
#if !defined(PS5_HOST_REFERENCE)
    pthread_t thread{};
    bool threadStarted{false};
    std::atomic<bool> finished{false};
    std::atomic<bool> cancelRequested{false};
    std::atomic<int> requestId{-1};
    int workerResult{0};
    int httpStatus{0};
    bool isSearch{false};
    bool isCover{false};
    Downloader* owner{nullptr};
    std::string url;
    std::vector<std::string> urls;
    std::string workerError;
    std::string coverPath;
    std::string coverFinalPath;
    std::string postBody;
    std::string response;
    FILE* output{nullptr};
    uint64_t received{0};
#endif
};

namespace {
// Search rows include issue reports and note metadata, so ordinary queries can
// exceed 1 MiB even though the browser only consumes name, artist, hash and
// video presence. Keep a hard ceiling while allowing those complete API rows.
constexpr size_t MaxSearchBytes = 8 * 1024 * 1024;
constexpr uint64_t MaxCoverBytes = 4ull * 1024ull * 1024ull;
constexpr uint64_t MaxChartBytes = 256ull * 1024ull * 1024ull;
constexpr uint64_t MaxExpandedBytes = 384ull * 1024ull * 1024ull;
constexpr uint64_t MaxSngHeaderBytes = 16ull * 1024ull * 1024ull;
constexpr uint64_t MaxSingleFileBytes = 256ull * 1024ull * 1024ull;
constexpr uint64_t MaxFileCount = 2048;

#if !defined(PS5_HOST_REFERENCE)
int ps5HttpContextId = -1;
bool initializePs5Network(const char*& error) noexcept {
    static pthread_mutex_t networkLock=PTHREAD_MUTEX_INITIALIZER;
    struct Lock {pthread_mutex_t* p;~Lock(){pthread_mutex_unlock(p);}} lock{&networkLock};
    pthread_mutex_lock(&networkLock);
    static bool initialized = false;
    static int poolId = -1;
    static int sslId = -1;
    if (initialized) return true;
    if (sceNetInit() != 0) { error = "sceNetInit failed."; return false; }
    poolId = sceNetPoolCreate("CloneHeroEncore", 32 * 1024, 0);
    if (poolId < 0) { error = "sceNetPoolCreate failed."; return false; }
    sslId = sceSslInit(256 * 1024);
    if (sslId < 0) { error = "sceSslInit failed."; return false; }
    ps5HttpContextId = sceHttp2Init(poolId, sslId, 256 * 1024, 1);
    if (ps5HttpContextId < 0) { error = "sceHttp2Init failed."; return false; }
    initialized = true;
    return true;
}
#endif

bool ensureDirectory(const char* path) noexcept {
    if (!path || !*path) return false;
    char buffer[512]{};
    const size_t len = std::strlen(path);
    if (len >= sizeof(buffer)) return false;
    std::memcpy(buffer, path, len + 1);
    for (size_t i = 1; i <= len; ++i) {
        if (buffer[i] != '/' && buffer[i] != '\0') continue;
        const char saved = buffer[i];
        buffer[i] = '\0';
        if (mkdir(buffer, 0775) != 0 && errno != EEXIST) return false;
        buffer[i] = saved;
    }
    DIR* directory = opendir(path);
    if (!directory) return false;
    closedir(directory);
    return true;
}

bool hasDirectChartFile(const char* path) noexcept {
    if (!path || !*path) return false;
    DIR* dir = opendir(path);
    if (!dir) return false;
    bool found = false;
    while (dirent* entry = readdir(dir)) {
        const size_t len = std::strlen(entry->d_name);
        if (len < 6) continue;
        const char* ext = entry->d_name + len - 6;
        if (std::tolower(static_cast<unsigned char>(ext[0])) != '.' ||
            std::tolower(static_cast<unsigned char>(ext[1])) != 'c' ||
            std::tolower(static_cast<unsigned char>(ext[2])) != 'h' ||
            std::tolower(static_cast<unsigned char>(ext[3])) != 'a' ||
            std::tolower(static_cast<unsigned char>(ext[4])) != 'r' ||
            std::tolower(static_cast<unsigned char>(ext[5])) != 't') continue;
        char candidate[768];
        const int n = std::snprintf(candidate, sizeof(candidate), "%s/%s", path, entry->d_name);
        if (n > 0 && static_cast<size_t>(n) < sizeof(candidate)) {
            DIR* chartDirectory = opendir(candidate);
            if (chartDirectory) { closedir(chartDirectory); continue; }
            FILE* chart = std::fopen(candidate, "rb");
            if (chart) {
                std::fclose(chart);
                found = true;
                break;
            }
        }
    }
    closedir(dir);
    return found;
}

void removeTree(const char* path) noexcept {
    DIR* dir = opendir(path);
    if (!dir) { unlink(path); return; }
    while (dirent* entry = readdir(dir)) {
        if (std::strcmp(entry->d_name, ".") == 0 || std::strcmp(entry->d_name, "..") == 0) continue;
        char child[768];
        const int n = std::snprintf(child, sizeof(child), "%s/%s", path, entry->d_name);
        if (n <= 0 || static_cast<size_t>(n) >= sizeof(child)) continue;
        DIR* childDirectory = opendir(child);
        if (childDirectory) {
            closedir(childDirectory);
            removeTree(child);
        } else {
            (void)unlink(child);
        }
    }
    closedir(dir);
    rmdir(path);
}

bool safeRelativePath(const std::string& value) noexcept {
    if (value.empty() || value.size() > 240 || value.front() == '/' || value.front() == '\\') return false;
    size_t segment = 0;
    for (size_t i = 0; i <= value.size(); ++i) {
        if (i < value.size()) {
            const unsigned char c = static_cast<unsigned char>(value[i]);
            if (c < 0x20 || c == 0x7f || c == ':' || c == '\\') return false;
        }
        if (i == value.size() || value[i] == '/') {
            const size_t count = i - segment;
            if (!count || (count == 1 && value[segment] == '.') ||
                (count == 2 && value[segment] == '.' && value[segment + 1] == '.')) return false;
            segment = i + 1;
        }
    }
    return true;
}

bool isMd5(const std::string& s) noexcept {
    if (s.size() != 32) return false;
    for (char c : s) if (!std::isxdigit(static_cast<unsigned char>(c))) return false;
    return true;
}

std::string safeFolderName(const std::string& value) {
    std::string out;
    out.reserve(48);
    for (unsigned char c : value) {
        if (out.size() >= 40) break;
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '-' || c == '_') out.push_back(static_cast<char>(c));
        else if (c == ' ' && !out.empty() && out.back() != '_') out.push_back('_');
    }
    while (!out.empty() && out.back() == '_') out.pop_back();
    return out.empty() ? "chart" : out;
}

} // namespace

bool httpGet(const std::string& url,HttpSink sink,void* user,uint64_t limit,
             const std::atomic<bool>& cancel,std::string& error) noexcept {
    error.clear();
#if defined(PS5_HOST_REFERENCE)
    (void)url;(void)sink;(void)user;(void)limit;(void)cancel;
    error="Networking is unavailable in this reference build.";return false;
#else
    if(url.rfind("https://",0)!=0 || !sink || !limit){error="Invalid HTTPS update request.";return false;}
    const char* initError=nullptr;if(!initializePs5Network(initError)){error=initError;return false;}
    int templ=sceHttp2CreateTemplate(ps5HttpContextId,"CloneHeroEncore-PS5-Updater/1",3,1);
    int request=-1;bool ok=false;
    if(templ>=0) {
        request=sceHttp2CreateRequestWithURL(templ,"GET",url.c_str(),0);
        if(request>=0) {
            (void)sceHttp2SetAutoRedirect(request,1);
            (void)sceHttp2SetConnectTimeOut(request,10000000);
            (void)sceHttp2SetSendTimeOut(request,10000000);
            (void)sceHttp2SetRecvTimeOut(request,10000000);
            (void)sceHttp2AddRequestHeader(request,"Accept","application/vnd.github+json",0);
            int status=0;
            if(!cancel && sceHttp2SendRequest(request,nullptr,0)>=0 && sceHttp2GetStatusCode(request,&status)>=0 && status==200) {
                std::array<uint8_t,32768> bytes{};uint64_t received=0;
                for(;;) {
                    if(cancel)break;
                    int count=sceHttp2ReadData(request,bytes.data(),bytes.size());
                    if(count==0){ok=true;break;}
                    if(count<0 || uint64_t(count)>limit-received || !sink(bytes.data(),size_t(count),user))break;
                    received+=unsigned(count);
                }
            } else {char text[80];std::snprintf(text,sizeof(text),"Update server unavailable (HTTP %d).",status);error=text;}
            (void)sceHttp2DeleteRequest(request);
        }
        (void)sceHttp2DeleteTemplate(templ);
    }
    if(!ok && error.empty())error=cancel?"Update cancelled.":"Update transfer failed or exceeded its size limit.";
    return ok;
#endif
}

Downloader::~Downloader() noexcept { cancel(); }

void Downloader::fail(const char* message) noexcept {
    error_ = message ? message : "Downloader failed.";
    state_ = State::Error;
}

#if !defined(PS5_HOST_REFERENCE)
void* Downloader::transferThread(void* user) noexcept {
    auto* transfer = static_cast<HttpTransfer*>(user);
    if (!transfer) return nullptr;
    const bool package=!transfer->isSearch&&!transfer->isCover;
    for(size_t attempt=0;attempt<std::max<size_t>(1,transfer->urls.size());++attempt) {
    if(transfer->cancelRequested.load(std::memory_order_relaxed))break;
    if(!transfer->urls.empty())transfer->url=transfer->urls[attempt];
    transfer->workerResult=0;transfer->httpStatus=0;transfer->received=0;
    int templateId = sceHttp2CreateTemplate(ps5HttpContextId,
        "CloneHeroPS5/0.1 (+native song browser)", 3, 1);
    int requestId = -1;
    if (templateId < 0) transfer->workerResult = templateId;
    if (transfer->workerResult == 0 && !transfer->cancelRequested.load(std::memory_order_relaxed)) {
        requestId = sceHttp2CreateRequestWithURL(templateId,
            transfer->isSearch ? "POST" : "GET", transfer->url.c_str(),
            transfer->isSearch ? static_cast<uint64_t>(transfer->postBody.size()) : 0);
        if (requestId < 0) transfer->workerResult = requestId;
    }
    if (requestId >= 0) {
        transfer->requestId.store(requestId, std::memory_order_release);
        (void)sceHttp2SetConnectTimeOut(requestId, 15ull * 1000ull * 1000ull);
        (void)sceHttp2SetSendTimeOut(requestId, 15ull * 1000ull * 1000ull);
        (void)sceHttp2SetRecvTimeOut(requestId, 15ull * 1000ull * 1000ull);
        if (transfer->isSearch && sceHttp2AddRequestHeader(requestId,
                "Content-Type", "application/json", 0) < 0) {
            transfer->workerResult = -1;
        }
        if (transfer->workerResult == 0 && !transfer->cancelRequested.load(std::memory_order_relaxed)) {
            const void* body = transfer->isSearch ? transfer->postBody.data() : nullptr;
            const size_t bodySize = transfer->isSearch ? transfer->postBody.size() : 0;
            transfer->workerResult = sceHttp2SendRequest(requestId, body, bodySize);
        }
        if (transfer->workerResult == 0 && !transfer->cancelRequested.load(std::memory_order_relaxed)) {
            transfer->workerResult = sceHttp2GetStatusCode(requestId, &transfer->httpStatus);
        }
        if (transfer->workerResult == 0 && transfer->httpStatus >= 200 && transfer->httpStatus < 300) {
            std::array<char, 16 * 1024> buffer{};
            for (;;) {
                if (transfer->cancelRequested.load(std::memory_order_relaxed)) { transfer->workerResult = -1; break; }
                const int length = sceHttp2ReadData(requestId, buffer.data(), buffer.size());
                if (length == 0) break;
                if (length < 0) { transfer->workerResult = length; break; }
                const uint64_t received = transfer->received + static_cast<uint64_t>(length);
                if (transfer->isSearch) {
                    if (received > MaxSearchBytes) { transfer->workerResult = -1; break; }
                    transfer->response.append(buffer.data(), static_cast<size_t>(length));
                } else {
                    const uint64_t maxBytes = transfer->isCover ? MaxCoverBytes : MaxChartBytes;
                    if (received > maxBytes || !transfer->output ||
                        std::fwrite(buffer.data(), 1, static_cast<size_t>(length), transfer->output) != static_cast<size_t>(length)) {
                        transfer->workerResult = -1;
                        break;
                    }
                    if (transfer->owner && !transfer->isCover)
                        transfer->owner->progress_.store(std::clamp(static_cast<double>(received) / static_cast<double>(MaxChartBytes), 0.0, 1.0), std::memory_order_relaxed);
                }
                transfer->received = received;
            }
        } else if (transfer->workerResult == 0) {
            transfer->workerResult = -transfer->httpStatus;
        }
        transfer->requestId.store(-1, std::memory_order_release);
        (void)sceHttp2DeleteRequest(requestId);
    }
    if (templateId >= 0) (void)sceHttp2DeleteTemplate(templateId);
    if(package && transfer->workerResult==0) {
        bool ok=transfer->output && std::fflush(transfer->output)==0 && fsync(fileno(transfer->output))==0;
        if(transfer->output && std::fclose(transfer->output)!=0)ok=false;
        transfer->output=nullptr;
        if(ok) {
            transfer->owner->progress_.store(0.95,std::memory_order_relaxed);
            const auto& chart=transfer->owner->results_[transfer->owner->selectedResult_];
            const PackageMetadata metadata{chart.title,chart.artist,chart.charter};
            ok=installSongPackage(transfer->owner->temporarySng_,transfer->owner->stageDirectory_,
                transfer->owner->finalDirectory_,transfer->workerError,&transfer->cancelRequested,&metadata);
        } else transfer->workerError="Could not flush the downloaded package.";
        if(!ok)transfer->workerResult=-1;
    }
    if(transfer->workerResult==0 || !package || (transfer->httpStatus!=404 && transfer->httpStatus!=410) || attempt+1>=transfer->urls.size())break;
    // Retry the original ZIP only when the hosted package is unavailable.
    if(transfer->output)std::fclose(transfer->output);
    transfer->output=std::fopen(transfer->owner->temporarySng_.c_str(),"wb");
    if(!transfer->output){transfer->workerResult=-1;break;}
    }
    transfer->finished.store(true, std::memory_order_release);
    return nullptr;
}

bool Downloader::beginTransfer(const char* url, const std::string* postBody, const char* outputPath) noexcept {
    if (!url || !*url || transfer_) return false;
    const char* networkError = nullptr;
    if (!initializePs5Network(networkError)) { fail(networkError ? networkError : "Could not initialize PS5 networking."); return false; }
    if (std::strncmp(url, "https://", 8) != 0) { fail("Only HTTPS URLs are supported."); return false; }
    transfer_ = new HttpTransfer{};
    if (!transfer_) { fail("Could not allocate network request."); return false; }
    transfer_->url = url;
    if(!postBody && !outputPath && selectedResult_<resultCount_) {
        transfer_->urls.push_back(url);
        const auto& chart=results_[selectedResult_];
        if(chart.hasVideo) {
            std::string full=url;const auto at=full.find("_novideo.sng");
            if(at!=std::string::npos){full.replace(at,12,".sng");transfer_->urls.push_back(full);}
        }
        if(!chart.zipFallbackUrl.empty())transfer_->urls.push_back(chart.zipFallbackUrl);
    }
    transfer_->isSearch = postBody != nullptr;
    transfer_->isCover = outputPath != nullptr;
    if (outputPath) transfer_->coverPath = outputPath;
    transfer_->owner = this;
    if (postBody) transfer_->postBody = *postBody;
    // Open the transactional package before the worker can receive its first
    // response bytes. This also keeps all file writes off the render thread.
    if (!transfer_->isSearch) {
        const char* destination = transfer_->isCover ? transfer_->coverPath.c_str() : temporarySng_.c_str();
        transfer_->output = std::fopen(destination, "wb");
        if (!transfer_->output) { fail("Cannot create temporary chart package."); cancel(); return false; }
    }
    pthread_attr_t attributes;
    if (pthread_attr_init(&attributes) != 0) { fail("Could not initialize the network worker."); cancel(); return false; }
    (void)pthread_attr_setstacksize(&attributes, 256u * 1024u);
    const int threadResult = pthread_create(&transfer_->thread, &attributes, &Downloader::transferThread, transfer_);
    (void)pthread_attr_destroy(&attributes);
    if (threadResult != 0) { fail("Could not start the network worker."); cancel(); return false; }
    transfer_->threadStarted = true;
    return true;
}

bool Downloader::coverPending(size_t resultIndex) const noexcept {
    if (resultIndex >= resultCount_ || !transfer_ || !transfer_->isCover) return false;
    return transfer_->coverFinalPath == results_[resultIndex].albumArtPath;
}

bool Downloader::requestCover(size_t resultIndex) noexcept {
    if (resultIndex >= resultCount_) return false;
    ChartResult& chart = results_[resultIndex];
    if (chart.albumArtMd5.size() != 32 || !isMd5(chart.albumArtMd5)) return false;
    if (chart.albumArtRequested) return access(chart.albumArtPath.c_str(), F_OK) == 0;
    static constexpr char cacheRoot[] = CH_SONGS "/.encore-covers";
    if (!ensureDirectory(cacheRoot)) return false;
    char imagePath[256], partialPath[272];
    const int n = std::snprintf(imagePath, sizeof(imagePath), "%s/%s.jpg", cacheRoot, chart.albumArtMd5.c_str());
    const int p = std::snprintf(partialPath, sizeof(partialPath), "%s.part", imagePath);
    if (n <= 0 || p <= 0 || static_cast<size_t>(n) >= sizeof(imagePath) || static_cast<size_t>(p) >= sizeof(partialPath)) return false;
    chart.albumArtPath = imagePath;
    if (access(chart.albumArtPath.c_str(), F_OK) == 0) return true;
    if (transfer_) return transfer_->isCover && transfer_->coverFinalPath == chart.albumArtPath;
    if (state_ == State::Searching || state_ == State::Downloading) return false;
    unlink(partialPath);
    const std::string url = std::string("https://files.enchor.us/") + chart.albumArtMd5 + ".jpg";
    if (!beginTransfer(url.c_str(), nullptr, partialPath)) return false;
    transfer_->coverFinalPath = imagePath;
    chart.albumArtRequested = true;
    return true;
}

bool Downloader::search(const char* keywords, unsigned page, SearchOptions options) noexcept {
    // Searching is an explicit user action, but it must not discard a package
    // transfer already in progress. Circle/Cancel remains the cancellation path.
    if (state_ == State::Downloading) return false;
    cancel();
    error_.clear(); installedPath_.clear(); response_.clear(); progress_ = 0.0;
    if (!keywords || !*keywords || std::strlen(keywords) > 96) { fail("Enter up to 96 characters to search."); return false; }
    // A completed install leaves the visible search results intact. Allow the
    // user to continue paging from that state as well as directly from Results.
    const bool hasSearchResults = state_ == State::Results || state_ == State::Complete;
    if (page == 0 || page > MaxPages ||
        (page > 1 && (!hasSearchResults || page != page_ + 1 || !hasMoreResults_))) {
        fail("There are no more chart results to load."); return false;
    }
    if (page == 1) {
        resultCount_ = 0;
        resultGroupCount_ = 0;
        resultGroups_.fill(std::string{});
        totalResults_ = 0;
        page_ = 0;
        hasMoreResults_ = false;
    }
    if (page == 1) searchOptions_ = options;
    const std::string sortDirection = searchOptions_.sortDescending ? "desc" : "asc";
    nlohmann::json request = {
        {"page", page}, {"per_page", 20},
        {"instrument", searchOptions_.instrument}, {"difficulty", searchOptions_.difficulty},
        {"drumType", searchOptions_.drumType}, {"drumsReviewed", searchOptions_.drumsReviewed},
        {"sort", {{"type", searchOptions_.sortField}, {"direction", sortDirection}}},
        {"source", "bridge"}
    };
    const char* endpoint = "https://api.enchor.us/search";
    if (searchOptions_.searchField && *searchOptions_.searchField) {
        static constexpr const char* textFields[] = {"name", "artist", "album", "charter", "genre", "year"};
        bool validField = false;
        for (const char* field : textFields) {
            if (std::strcmp(field, searchOptions_.searchField) == 0) {
                request[field] = {{"value", keywords}, {"exact", false}, {"exclude", false}};
                validField = true;
            } else request[field] = nullptr;
        }
        if (!validField) { fail("Invalid advanced search field."); return false; }
        static constexpr const char* optionalFilters[] = {
            "minLength", "maxLength", "minIntensity", "maxIntensity", "minAverageNPS", "maxAverageNPS",
            "minMaxNPS", "maxMaxNPS", "minYear", "maxYear", "modifiedAfter", "hash", "chartHash",
            "trackHash", "hasSoloSections", "hasForcedNotes", "hasOpenNotes", "hasTapNotes", "hasLyrics",
            "hasVocals", "hasRollLanes", "has2xKick", "hasIssues", "hasVideoBackground", "modchart", "chartIdAfter"
        };
        for (const char* filter : optionalFilters) request[filter] = nullptr;
        endpoint = "https://api.enchor.us/search/advanced";
    } else {
        request["search"] = keywords;
    }
    postBody_ = request.dump();
    requestedPage_ = page;
    state_ = State::Searching;
    if (!beginTransfer(endpoint, &postBody_)) return false;
    return true;
}

bool Downloader::loadNextPage(const char* keywords) noexcept {
    const bool canContinueSearch = state_ == State::Results || state_ == State::Complete;
    return canContinueSearch && hasMoreResults_ && page_ < MaxPages
        ? search(keywords, page_ + 1, searchOptions_) : false;
}

bool Downloader::download(size_t resultIndex) noexcept {
    if (resultIndex >= resultCount_ || (state_ != State::Results && state_ != State::Complete)) return false;
    cancel();
    error_.clear(); installedPath_.clear(); progress_ = 0.0; selectedResult_ = resultIndex;
    const ChartResult& chart = results_[resultIndex];
    if (!isMd5(chart.md5)) { fail("The selected chart has an invalid identifier."); return false; }
    if (!ensureDirectory(songRoot_.c_str())) { fail("Cannot create the console songs folder."); return false; }
    char md5[33]{};
    std::snprintf(md5, sizeof(md5), "%s", chart.md5.c_str());
    char temporary[512], stage[512], final[512];
    const int a = std::snprintf(temporary, sizeof(temporary), "%s/.%s.package.part",songRoot_.c_str(),md5);
    const int b = std::snprintf(stage, sizeof(stage), "%s/.install-%s",songRoot_.c_str(),md5);
    const std::string slug = safeFolderName(chart.title);
    const int c = std::snprintf(final, sizeof(final), "%s/%s_%.8s",songRoot_.c_str(),slug.c_str(),md5);
    if (a <= 0 || b <= 0 || c <= 0 || static_cast<size_t>(a) >= sizeof(temporary) ||
        static_cast<size_t>(b) >= sizeof(stage) || static_cast<size_t>(c) >= sizeof(final)) {
        fail("Song folder path is too long."); return false;
    }
    DIR* existingDirectory = opendir(final);
    if (existingDirectory) {
        closedir(existingDirectory);
        if (hasDirectChartFile(final)) {
            installedPath_ = final;
            state_ = State::Complete;
            return true;
        }
        fail("A conflicting or incomplete item already uses the chart folder name.");
        return false;
    }
    // This runtime can report ENOTDIR for a missing child beneath /app0 even
    // when opendir succeeds on the parent. Treat it like ENOENT here.
    if (errno != ENOENT && errno != ENOTDIR) { fail("Cannot inspect the chart destination folder."); return false; }
    temporarySng_ = temporary; stageDirectory_ = stage; finalDirectory_ = final;
    unlink(temporarySng_.c_str());
    removeTree(stageDirectory_.c_str());
    if (mkdir(stageDirectory_.c_str(), 0775) != 0) { fail("Cannot create chart install staging folder."); return false; }
    transfer_ = nullptr;
    state_ = State::Downloading;
    const std::string url = std::string("https://files.enchor.us/") +
        (chart.encoderVersion?"charts/v"+std::to_string(chart.encoderVersion)+"/":std::string{}) + chart.md5 + (chart.hasVideo ? "_novideo.sng" : ".sng");
    if (!beginTransfer(url.c_str(), nullptr)) { removeTree(stageDirectory_.c_str()); return false; }
    return true;
}

void Downloader::update() noexcept {
    if (!transfer_) return;
    if (!transfer_->finished.load(std::memory_order_acquire)) return;
    if (transfer_->threadStarted) {
        (void)pthread_join(transfer_->thread, nullptr);
        transfer_->threadStarted = false;
    }
    if (transfer_->workerResult != 0) {
        if (transfer_->isCover) {
            const std::string partial = transfer_->coverPath;
            cancel();
            unlink(partial.c_str());
            return;
        }
        const bool downloading = state_ == State::Downloading;
        const int status = transfer_->httpStatus;
        const std::string error=transfer_->workerError;
        fail(!error.empty()?error.c_str():status==404||status==410?"Neither the hosted package nor an available original ZIP could be downloaded.":
             (status>=400?"The server rejected the request.":"HTTPS transfer failed."));
        cancel();
        if (downloading) { unlink(temporarySng_.c_str()); removeTree(stageDirectory_.c_str()); }
        return;
    }
    if (transfer_->isCover) {
        bool ok = transfer_->output && std::fflush(transfer_->output) == 0 && fsync(fileno(transfer_->output)) == 0;
        if (transfer_->output && std::fclose(transfer_->output) != 0) ok = false;
        transfer_->output = nullptr;
        const std::string partial = transfer_->coverPath;
        const std::string finalPath = transfer_->coverFinalPath;
        transfer_->isCover=false; // The successful cover is committed below.
        cancel();
        if (ok && rename(partial.c_str(), finalPath.c_str()) != 0) unlink(partial.c_str());
        else if (!ok) unlink(partial.c_str());
        return;
    }
    if (state_ == State::Searching) response_ = std::move(transfer_->response);
    finishTransfer(transfer_->httpStatus);
    if (state_ == State::Error && !temporarySng_.empty()) {
        unlink(temporarySng_.c_str()); removeTree(stageDirectory_.c_str());
    }
}

void Downloader::finishTransfer(long) noexcept {
    const bool wasSearch = state_ == State::Searching;
    if (transfer_ && transfer_->output) {
        bool ok = std::fflush(transfer_->output) == 0 && fsync(fileno(transfer_->output)) == 0;
        if (std::fclose(transfer_->output) != 0) ok = false;
        transfer_->output = nullptr;
        if (!ok) {
            fail("Could not flush the downloaded chart package."); cancel();
            unlink(temporarySng_.c_str()); removeTree(stageDirectory_.c_str());
            return;
        }
    }
    state_ = wasSearch ? State::Results : State::Complete;
    cancel();
    if (wasSearch) {
        if (!parseSearchResponse()) { fail("The search response was invalid."); return; }
        state_ = State::Results;
        return;
    }
    if (state_ == State::Error) { unlink(temporarySng_.c_str()); removeTree(stageDirectory_.c_str()); return; }
    // The joined worker has already extracted, normalized and committed it.
    installedPath_=finalDirectory_;
    unlink(temporarySng_.c_str());
    state_ = State::Complete;
    progress_ = 1.0;
}

bool Downloader::parseSearchResponse() noexcept {
    const auto document = nlohmann::json::parse(response_.begin(), response_.end(), nullptr, false);
    if (document.is_discarded() || !document.is_object()) return false;
    const auto dataIt = document.find("data");
    if (dataIt == document.end() || !dataIt->is_array()) return false;
    const auto foundIt = document.find("found");
    if (foundIt == document.end() || !foundIt->is_number_unsigned() && !foundIt->is_number_integer()) return false;
    const auto found = foundIt->get<int64_t>();
    if (found < 0) return false;
    totalResults_ = static_cast<size_t>(found);
    if (requestedPage_ == 1) {
        resultCount_ = 0;
        resultGroupCount_ = 0;
        resultGroups_.fill(std::string{});
    }
    for (const auto& item : *dataIt) {
        if (!item.is_object() || resultCount_ >= MaxResults) break;
        const auto md5It = item.find("md5");
        auto titleIt = item.find("name");
        if (titleIt == item.end()) titleIt = item.find("title");
        if (md5It == item.end() || !md5It->is_string() || titleIt == item.end() || !titleIt->is_string()) continue;
        ChartResult entry;
        const auto encoder=item.find("encoderVersion");
        if(encoder!=item.end()&&encoder->is_number_integer()) {
            const int64_t version=encoder->get<int64_t>();
            if(version>=0&&version<=1000000)entry.encoderVersion=static_cast<unsigned>(version);
        }
        const auto drive=item.find("driveFileId"),filename=item.find("driveFileName"),pack=item.find("driveChartIsPack");
        if(drive!=item.end()&&drive->is_string()&&filename!=item.end()&&filename->is_string() &&
           !(pack!=item.end()&&pack->is_boolean()&&pack->get<bool>())) {
            std::string name=filename->get<std::string>();for(char& c:name)c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            const std::string id=drive->get<std::string>();
            const bool valid=!id.empty()&&id.size()<=160&&std::all_of(id.begin(),id.end(),[](unsigned char c){return std::isalnum(c)||c=='_'||c=='-';});
            if(valid&&name.size()>4&&name.substr(name.size()-4)==".zip")
                entry.zipFallbackUrl="https://drive.usercontent.google.com/download?export=download&confirm=t&id="+id;
        }
        entry.md5 = md5It->get<std::string>();
        entry.title = titleIt->get<std::string>();
        if (!isMd5(entry.md5) || entry.title.empty()) continue;
        if (entry.title.size() > 160) entry.title.resize(160);
        const auto groupIt = item.find("groupId");
        if (groupIt != item.end() && (groupIt->is_number_unsigned() || groupIt->is_number_integer())) {
            entry.groupKey = std::to_string(groupIt->get<int64_t>());
        } else {
            // Older or alternate API responses may omit groupId; keep their
            // records navigable by treating each distinct chart as one group.
            entry.groupKey = entry.md5;
        }
        const auto artistIt = item.find("artist");
        if (artistIt != item.end() && artistIt->is_string()) {
            entry.artist = artistIt->get<std::string>();
            if (entry.artist.size() > 120) entry.artist.resize(120);
        }
        const auto charterIt = item.find("charter");
        if (charterIt != item.end() && charterIt->is_string()) {
            entry.charter = charterIt->get<std::string>();
            if (entry.charter.size() > 120) entry.charter.resize(120);
        }
        const auto albumArtIt = item.find("albumArtMd5");
        if (albumArtIt != item.end() && albumArtIt->is_string()) {
            entry.albumArtMd5 = albumArtIt->get<std::string>();
            if (!isMd5(entry.albumArtMd5)) entry.albumArtMd5.clear();
        }
        const auto notesDataIt = item.find("notesData");
        if (notesDataIt != item.end() && notesDataIt->is_object()) {
            static constexpr const char* instrumentNames[] = {
                "guitar", "guitarcoop", "rhythm", "bass", "drums", "keys",
                "guitarghl", "guitarcoopghl", "rhythmghl", "bassghl"
            };
            const auto countsIt = notesDataIt->find("noteCounts");
            if (countsIt != notesDataIt->end() && countsIt->is_array()) {
                for (const auto& noteCount : *countsIt) {
                    if (!noteCount.is_object()) continue;
                    const auto instrumentIt = noteCount.find("instrument");
                    const auto countIt = noteCount.find("count");
                    if (instrumentIt == noteCount.end() || !instrumentIt->is_string() ||
                        countIt == noteCount.end() || !countIt->is_number() || countIt->get<double>() <= 0.0) continue;
                    size_t instrumentIndex = std::size(instrumentNames);
                    for (size_t n = 0; n < std::size(instrumentNames); ++n) {
                        if (instrumentIt->get<std::string>() == instrumentNames[n]) {
                            instrumentIndex = n;
                            break;
                        }
                    }
                    if (instrumentIndex >= std::size(instrumentNames)) continue;
                    entry.chartedInstrumentMask |= static_cast<uint16_t>(1u << instrumentIndex);
                    const auto difficultyIt = noteCount.find("difficulty");
                    if (difficultyIt == noteCount.end() || !difficultyIt->is_string()) continue;
                    const std::string difficulty = difficultyIt->get<std::string>();
                    uint8_t difficultyBit = 0;
                    if (difficulty == "expert") difficultyBit = 1u << 0;       // E
                    else if (difficulty == "medium") difficultyBit = 1u << 1; // M
                    else if (difficulty == "hard") difficultyBit = 1u << 2;   // H
                    else if (difficulty == "easy") difficultyBit = 1u << 3;   // X
                    entry.instrumentDifficultyMasks[instrumentIndex] |= difficultyBit;
                }
            }
            const auto vocalsIt = notesDataIt->find("hasVocals");
            entry.hasVocals = vocalsIt != notesDataIt->end() && vocalsIt->is_boolean() && vocalsIt->get<bool>();
        }
        const auto videoIt = item.find("hasVideoBackground");
        entry.hasVideo = videoIt != item.end() && videoIt->is_boolean() && videoIt->get<bool>();
        bool groupSeen = false;
        for (size_t group = 0; group < resultGroupCount_; ++group) {
            if (resultGroups_[group] == entry.groupKey) {
                groupSeen = true;
                break;
            }
        }
        if (!groupSeen && resultGroupCount_ < resultGroups_.size())
            resultGroups_[resultGroupCount_++] = entry.groupKey;
        results_[resultCount_++] = std::move(entry);
    }
    page_ = requestedPage_;
    hasMoreResults_ = resultGroupCount_ < totalResults_ && resultCount_ < MaxResults && page_ < MaxPages;
    return true;
}


#else

bool Downloader::search(const char*, unsigned, SearchOptions) noexcept { fail("The song browser is available in the PS5 build."); return false; }
bool Downloader::loadNextPage(const char*) noexcept { return false; }
bool Downloader::download(size_t) noexcept { fail("The song browser is available in the PS5 build."); return false; }
void Downloader::update() noexcept {}
void Downloader::finishTransfer(long) noexcept {}
bool Downloader::beginTransfer(const char*, const std::string*, const char*) noexcept { return false; }
bool Downloader::requestCover(size_t) noexcept { return false; }
bool Downloader::coverPending(size_t) const noexcept { return false; }
bool Downloader::parseSearchResponse() noexcept { return false; }

#endif

// Pure result accessor, available even in the offline Linux graphics build.
const char* Downloader::coverPath(size_t resultIndex) const noexcept {
    if (resultIndex >= resultCount_) return "";
    return results_[resultIndex].albumArtPath.c_str();
}

void Downloader::reset() noexcept {
    cancel();
    state_=State::Idle;error_.clear();installedPath_.clear();response_.clear();postBody_.clear();
    resultCount_=selectedResult_=totalResults_=resultGroupCount_=0;
    page_=requestedPage_=0;hasMoreResults_=false;progress_=0;
    results_.fill(ChartResult{});resultGroups_.fill(std::string{});
    temporarySng_.clear();stageDirectory_.clear();finalDirectory_.clear();
}

void Downloader::cancel() noexcept {
    const bool abortingDownload = state_ == State::Downloading;
#if !defined(PS5_HOST_REFERENCE)
    if (transfer_) {
        if (transfer_->threadStarted) {
            transfer_->cancelRequested.store(true, std::memory_order_relaxed);
            const int requestId = transfer_->requestId.load(std::memory_order_acquire);
            if (requestId >= 0) (void)sceHttp2AbortRequest(requestId);
            (void)pthread_join(transfer_->thread, nullptr);
            transfer_->threadStarted = false;
        }
        if (transfer_->output) std::fclose(transfer_->output);
        if(transfer_->isCover&&!transfer_->coverPath.empty())unlink(transfer_->coverPath.c_str());
        delete transfer_;
        transfer_ = nullptr;
    }
#else
    delete transfer_;
    transfer_ = nullptr;
#endif
    if (abortingDownload) {
        if (!temporarySng_.empty()) unlink(temporarySng_.c_str());
        if (!stageDirectory_.empty()) removeTree(stageDirectory_.c_str());
        state_ = State::Idle;
    }
}

} // namespace chorus
