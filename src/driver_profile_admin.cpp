#include "DriverProfileDatabase.hpp"
#include "EncryptedProfileBundle.hpp"

#include <opencv2/imgcodecs.hpp>
#include <opencv2/videoio.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

namespace
{
using Args=std::map<std::string,std::string>;
Args parse(int argc,char**argv,std::string&command){if(argc<2)throw std::runtime_error("command required");command=argv[1];Args a;for(int i=2;i<argc;++i){std::string s=argv[i];auto p=s.find('=');if(p==std::string::npos||s.rfind("--",0)!=0)throw std::runtime_error("expected --name=value");if(!a.emplace(s.substr(2,p-2),s.substr(p+1)).second)throw std::runtime_error("duplicate option");}return a;}
std::string required(const Args&a,const char*n){auto i=a.find(n);if(i==a.end()||i->second.empty())throw std::runtime_error(std::string("missing --")+n);return i->second;}
std::vector<std::uint8_t> readFile(const std::filesystem::path&p){std::ifstream f(p,std::ios::binary);if(!f)throw std::runtime_error("unable to open input file");return {std::istreambuf_iterator<char>(f),{}};}
void writeFileAtomic(const std::filesystem::path&p,const std::vector<std::uint8_t>&b){auto t=p;t += ".new";{std::ofstream f(t,std::ios::binary|std::ios::trunc);if(!f||!f.write(reinterpret_cast<const char*>(b.data()),static_cast<std::streamsize>(b.size())))throw std::runtime_error("unable to write encrypted store");f.flush();if(!f)throw std::runtime_error("unable to flush encrypted store");}
#ifdef _WIN32
if(!MoveFileExW(t.c_str(),p.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)){std::filesystem::remove(t);throw std::runtime_error("unable to atomically replace encrypted store");}
#else
std::error_code ec;std::filesystem::rename(t,p,ec);if(ec){std::filesystem::remove(t);throw std::runtime_error("unable to atomically replace encrypted store");}
#endif
}
std::string passphrase(const char*prompt){std::cerr<<prompt;
#ifdef _WIN32
HANDLE input=GetStdHandle(STD_INPUT_HANDLE);DWORD mode=0;const bool console=input!=INVALID_HANDLE_VALUE&&GetConsoleMode(input,&mode);if(console)SetConsoleMode(input,mode&~ENABLE_ECHO_INPUT);
#else
termios oldMode{};const bool console=isatty(STDIN_FILENO)&&tcgetattr(STDIN_FILENO,&oldMode)==0;if(console){auto mode=oldMode;mode.c_lflag&=~ECHO;tcsetattr(STDIN_FILENO,TCSANOW,&mode);}
#endif
std::string p;std::getline(std::cin,p);
#ifdef _WIN32
if(console)SetConsoleMode(input,mode);
#else
if(console)tcsetattr(STDIN_FILENO,TCSANOW,&oldMode);
#endif
std::cerr<<'\n';if(p.size()<12)throw std::runtime_error("passphrase must contain at least 12 characters");return p;}
dms::DriverProfileDatabase load(const std::filesystem::path&p,const std::string&pass){if(!std::filesystem::exists(p))return {};std::string error;std::vector<std::uint8_t> plain;if(!dms::decryptProfileBundle(readFile(p),pass,plain,error))throw std::runtime_error(error);auto db=dms::DriverProfileDatabase::deserialize(plain,error);if(!db)throw std::runtime_error(error);return std::move(*db);}
void save(const std::filesystem::path&p,const std::string&pass,const dms::DriverProfileDatabase&db){std::string error;auto plain=db.serialize(error);std::vector<std::uint8_t> bundle;if(plain.empty()||!dms::encryptProfileBundle(plain,pass,{},bundle,error))throw std::runtime_error(error);writeFileAtomic(p,bundle);}
dms::EnrollmentSource source(const std::string&s){if(s=="photo")return dms::EnrollmentSource::Photo;if(s=="video")return dms::EnrollmentSource::Video;if(s=="live")return dms::EnrollmentSource::Live;throw std::runtime_error("source must be photo, video, or live");}
std::vector<std::uint8_t> capture(const std::string&s,const Args&a){cv::Mat frame;if(s=="photo")frame=cv::imread(required(a,"input"));else{cv::VideoCapture video;if(s=="live")video.open(std::stoi(a.count("camera")?a.at("camera"):"0"));else video.open(required(a,"input"));if(!video.isOpened())throw std::runtime_error("unable to open video/camera");for(int i=0;i<10;++i)if(!video.read(frame))break;}if(frame.empty())throw std::runtime_error("unable to obtain enrollment frame");std::vector<unsigned char> encoded;if(!cv::imencode(".jpg",frame,encoded,{cv::IMWRITE_JPEG_QUALITY,95}))throw std::runtime_error("unable to encode enrollment frame");return encoded;}
}

int main(int argc,char**argv)
{
 try{std::string command;auto args=parse(argc,argv,command);auto store=std::filesystem::path(required(args,"store"));auto pass=passphrase("Store passphrase: ");auto db=load(store,pass);std::string error;
  if(command=="init"){if(std::filesystem::exists(store))throw std::runtime_error("store already exists");auto confirmation=passphrase("Confirm passphrase: ");if(pass!=confirmation)throw std::runtime_error("passphrases do not match");save(store,pass,db);}
  else if(command=="list"){for(const auto&p:db.profiles())std::cout<<p.driverId<<'\t'<<p.displayName<<'\t'<<p.images.size()<<'\t'<<p.embeddings.size()<<'\n';return 0;}
  else if(command=="create"){if(!db.create(required(args,"driver-id"),args.count("display-name")?args.at("display-name"):"",error))throw std::runtime_error(error);save(store,pass,db);}
  else if(command=="delete"){if(!db.erase(required(args,"driver-id"),error))throw std::runtime_error(error);save(store,pass,db);}
  else if(command=="add-media"){auto kind=required(args,"source");float quality=std::stof(required(args,"quality"));dms::EnrollmentImage image{source(kind),quality,capture(kind,args)};if(!db.addImage(required(args,"driver-id"),std::move(image),error))throw std::runtime_error(error);save(store,pass,db);}
  else if(command=="export"){auto output=std::filesystem::path(required(args,"output"));if(output==store)throw std::runtime_error("export output must differ from store");auto bytes=readFile(store);std::vector<std::uint8_t> check;if(!dms::decryptProfileBundle(bytes,pass,check,error))throw std::runtime_error(error);writeFileAtomic(output,bytes);}
  else throw std::runtime_error("commands: init, list, create, delete, add-media, export");
  std::cout<<command<<" completed\n";return 0;
 }catch(const std::exception&e){std::cerr<<"driver profile admin failed: "<<e.what()<<'\n';return 1;}
}
