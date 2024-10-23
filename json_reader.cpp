#include "json_reader.hpp"

using namespace std;

namespace silicontrip {

 Json::Value json_reader::read_json_from_file(const string url)
{
    int pathsep = url.find_first_of('/');
    string path = url.substr(pathsep);

    Json::Value result;

    ifstream file(path);

    file >> result;

    return result;
}

 Json::Value json_reader::read_json_from_http(const string url) 
{

    Json::Value result;

    stringstream str_res;

    curlpp::options::Url thisUrl(url);
    curlpp::Easy thisRequest;

    thisRequest.setOpt(thisUrl);
    // str_res << curlpp::options::Url(url);
    // cerr << "response: " << str_res.str() << endl;

    curlpp::options::WriteStream ws(&str_res);
    thisRequest.setOpt(ws);
    thisRequest.perform();

    if(curlpp::infos::ResponseCode::get(thisRequest) == 500)
    {
        throw invalid_argument(str_res.str());
    }

    str_res << thisRequest;

    str_res >> result;

    return result;
}

}
