#include <iostream>
#include <string>
#include <curl/curl.h>
#include <windows.h>
#include <nlohmann/json.hpp>
#define NLOHMANN_JSON_HAS_HELPER(x)
#define NLOHMANN_BASIC_JSON_TPL_DECLARATION
#define NLOHMANN_BASIC_JSON_TPL_DEFAULT_TYPE \
    nlohmann::adl_serializer, std::allocator, nlohmann::unicode_traits<char>
#include <fstream>
#include <regex>
#include <vector>
#include <cstdlib>

size_t WriteCallbackDownload(void* contents, size_t size, size_t nmemb, void* userp) {
    std::ofstream* ofs = (std::ofstream*)userp;
    size_t len = size * nmemb;
    ofs->write((char*)contents, len);
    return len;
}

double get_audio_duration(const std::string& audio_file) {
    std::string command = "ffprobe -v error -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 " + audio_file;
    char buffer[128];
    std::string result = "";
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) throw std::runtime_error("popen() failed!");
    try {
        while (fgets(buffer, sizeof buffer, pipe) != NULL) {
            result += buffer;
        }
    } catch (...) {
        pclose(pipe);
        throw;
    }
    pclose(pipe);
    return std::stod(result);
}

void download_image(const std::string& url, const std::string& save_path) {
    CURL* curl;
    CURLcode res;

    curl_global_init(CURL_GLOBAL_DEFAULT);

    curl = curl_easy_init();
    if(curl) {
        std::ofstream ofs(save_path, std::ios::binary);

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallbackDownload);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ofs);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        res = curl_easy_perform(curl);

        if(res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
}

std::vector<std::string> getImagesPrompt(std::string message_from_gpt)
{

    std::regex rgx("\\{\\d+\\}\\s(.*?)\\s*\\{");
    std::smatch matches;

    std::string::const_iterator searchStart(message_from_gpt.cbegin());

    std::vector<std::string> texts;
    while (std::regex_search(searchStart, message_from_gpt.cend(), matches, rgx)) {
        texts.push_back(matches[1].str());
        searchStart += matches.position() + matches.length() - 1;
    }

    std::regex lastRgx("\\{\\d+\\}\\s(.*?$)");
    if (std::regex_search(searchStart, message_from_gpt.cend(), matches, lastRgx)) {
        texts.push_back(matches[1].str());
    }
    return texts;

}

size_t WriteCallback( void * contents, size_t size, size_t nmemb,void * userp)
{
    ((std::string * )userp)->append((char *)contents,size * nmemb);
    return size * nmemb;

}
void imageGenerator(const std::string& save_path,const std::string imageToGenerate)
{
    std::string readBuffer;

    CURL* curl;
    CURLcode res;

    curl_global_init(CURL_GLOBAL_DEFAULT);

    curl = curl_easy_init();
    if(curl) {
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, "Authorization: Bearer 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");

        curl_easy_setopt(curl, CURLOPT_URL, "https://api.openai.com/v1/images/generations");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        std::string json = "{\"prompt\": \"" + imageToGenerate + "\", \"n\": 1, \"size\": \"1024x1024\"}";

        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json.c_str());

        res = curl_easy_perform(curl);

        if(res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

        nlohmann::json j = nlohmann::json::parse(readBuffer);
        std::string url = j["data"][0]["url"];
        download_image(url,save_path);
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();


}

std::size_t callback(
    const char* in,
    std::size_t size,
    std::size_t num,
    std::ofstream* out)
{
    const std::size_t totalBytes(size * num);
    out->write(in, totalBytes);
    return totalBytes;
}

void getSpeechFromText(const std::string& text, const std::string& apiKey, const std::string& voiceId) {
    const std::string url = "https://api.elevenlabs.io/v1/text-to-speech/" + voiceId;
    const std::string contentType = "application/json";

    nlohmann::json postData;
    postData["text"] = text;
    postData["model_id"] = "eleven_multilingual_v1";
    postData["voice_settings"] = {{"stability", 0.5}, {"similarity_boost", 0.5}};

    std::string postDataStr = postData.dump();

    CURL* curl = curl_easy_init();

    if(!curl) {
        std::cerr << "Failed to initialize curl" << std::endl;
        return;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postDataStr.c_str());
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, ("xi-api-key: " + apiKey).c_str());
    headers = curl_slist_append(headers, ("Content-Type: " + contentType).c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    std::ofstream outputFile("output.mp3", std::ios::binary);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &outputFile);

    CURLcode res = curl_easy_perform(curl);
    if(res != CURLE_OK) {
        std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
    }

    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);

    outputFile.close();
}


std::string removeSpecialChars(const std::string &s) {
    std::string result = "";
    for (char c : s) {
        switch (c) {
            case '\\':
            case '\"':
            case '/':
            case '\b':
            case '\f':
            case '\n':
            case '\r':
            case '\t':
                break;
            default:
                result += c;
                break;
        }
    }
    return result;
}
std::string send_gpt_request(const std::string &api_key, const std::string &message )
{
    CURL * curl;
    CURLcode res;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    std::string readBuffer;
    curl = curl_easy_init();
    std::string response;

    if (curl)
    {
        std::string data = "{\"model\": \"gpt-3.5-turbo\", \"messages\": [{\"role\": \"user\", \"content\": \"" + message + "\"}], \"temperature\": 0.7}";
        //std::cout<<data<<std::endl;


        struct curl_slist * headers = NULL;
        headers = curl_slist_append(headers,"Content-Type: application/json");
        std::string api_key_header = "Authorization: Bearer " + api_key;

        headers = curl_slist_append(headers,api_key_header.c_str());
        curl_easy_setopt(curl,CURLOPT_URL,  "https://api.openai.com/v1/chat/completions");
        curl_easy_setopt(curl,CURLOPT_POSTFIELDS,data.c_str());
        curl_easy_setopt(curl,CURLOPT_HTTPHEADER,headers);
        curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,WriteCallback);
        curl_easy_setopt(curl,CURLOPT_WRITEDATA, &readBuffer);
            curl_easy_setopt(curl,CURLOPT_SSL_VERIFYPEER,0L);
       curl_easy_setopt(curl,CURLOPT_SSL_VERIFYHOST,0L);
         res = curl_easy_perform(curl);
         if(res != CURLE_OK)
            std::cerr << "Wystapil blad podczas komunikacji z gpt: " << curl_easy_strerror(res) << std::endl;
         else{
            nlohmann::json response_json = nlohmann::json::parse(readBuffer);
            std::string normalResponse = response_json["choices"][0]["message"]["content"].dump(-1, ' ', false, nlohmann::json::error_handler_t::replace);;
            return removeSpecialChars(normalResponse);
         }
         curl_easy_cleanup(curl);
         curl_slist_free_all(headers);


    }
    curl_global_cleanup();
    return readBuffer;

}
std::string getTextFromDiv(const std::string & html, const std::string& div_id)
{

  std::string start_pattern = "id=\"" + div_id + "\"";
    std::string end_tag = "</div>";

    std::size_t start_pos = html.find(start_pattern);
    if (start_pos == std::string::npos) {
        return "";
    }

    start_pos = html.find(">", start_pos) + 1;
    std::size_t end_pos = html.find(end_tag, start_pos);

    return html.substr(start_pos, end_pos - start_pos);
}

std::string fetchSubtitles(const std::string &url)
{
    CURL * curl;
    CURLcode res;
    std::string readBuffer;
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (curl)
    {
       curl_easy_setopt(curl,CURLOPT_URL, url.c_str());
       curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,WriteCallback);
       curl_easy_setopt(curl,CURLOPT_WRITEDATA, &readBuffer);

       curl_easy_setopt(curl,CURLOPT_SSL_VERIFYPEER,0L);
       curl_easy_setopt(curl,CURLOPT_SSL_VERIFYHOST,0L);
       res = curl_easy_perform(curl);

       if(res != CURLE_OK)
           std::cout<<"Wystapil blad podczas pobierania napisow!"<<std::endl;
        curl_easy_cleanup(curl);


    }
    curl_global_cleanup();

    return getTextFromDiv(readBuffer,"text");
}
std::string getYoutubeId(const std::string& url) {
    std::size_t start = url.find("v=") + 2;
    std::size_t end = url.find("&", start);
    return url.substr(start, end - start);
}

int main() {

    SetConsoleOutputCP(CP_UTF8);

    std::string gptApi;
    std::cout << "Please enter GPT API Key: ";
    std::cin >> gptApi;

    std::string ElevenApiKey;
    std::cout << "Please enter Eleven Labs API Key: ";
    std::cin >> ElevenApiKey;


      std::string youtubeURL;
    std::cout << "Please enter YouTube URL: ";
    std::cin >> youtubeURL;

    std::string youtubeID = getYoutubeId(youtubeURL);

    std::string url = "https://www.captionsgrabber.com/8302/display-captions-as-text.00.php?id=" + youtubeID + "&language=en&type=None";
    std::string htmlContent = fetchSubtitles(url);
    std::cout<<"Pobralem napisy!"<<std::endl;

    std::string message_to_gpt = "[Prosze przetlumaczyc nastepujacy tekst na jezyk polski i wstawic trzy sygnatury {1}, {2}, {3}, {4} w odpowiednich miejscach - sygnatury maja byc napisane po angielsku!!!. Po przetlumaczeniu prosze tylko i jedynie o wygenerowanie propozycji opisow dla zdjec, ktore powinny byc wstawione w miejscach oznaczonych sygnaturami {1}, {2}, {3},{4} Wazne Opisy tych zdjec (sygnatur) maja byc wylacznie w jezyku angielskim!!]" + removeSpecialChars(htmlContent);
    std::string message_from_gpt = send_gpt_request(gptApi, message_to_gpt);

    std::cout<<"Przetlumaczylem tekst!"<<std::endl;
    std::vector<std::string> imagesPrompt = getImagesPrompt(message_from_gpt);
    for(int i=0; i < 4; i++ )
    {
        std::string filename = "Obraz" + std::to_string(i+1) + ".jpg";
        imageGenerator(filename, imagesPrompt[i]);
    }
    std::cout<<"Wygenerowalem obrazy!"<<std::endl;



    const std::string voiceId = "ErXwobaYiN019PkySvjV";
    getSpeechFromText(message_from_gpt, ElevenApiKey, voiceId);
    std::cout<<"Wygenerowalem audio!"<<std::endl;

    double duration = get_audio_duration("output.mp3");
    double image_duration = duration / 4;
    std::string command = "ffmpeg -loop 1 -t " + std::to_string(image_duration) + " -i Obraz1.jpg -loop 1 -t " + std::to_string(image_duration) + " -i Obraz2.jpg -loop 1 -t " + std::to_string(image_duration) + " -i Obraz3.jpg -i output.mp3 -filter_complex \"[0:v][1:v][2:v]concat=n=3:v=1:a=0[v]\" -map \"[v]\" -map 3:a -shortest -acodec copy -vcodec mjpeg result.mkv";
    std::system(command.c_str());
    return 0;
}
