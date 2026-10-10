#include "trinity/web_runtime.hpp"
#include <cstdio>
#include <cstring>
using namespace trinity;
static const trinity::u8 kGzip[]={31,139,8,0,0,0,0,0,2,255,11,41,202,204,203,44,169,84,112,42,202,47,47,78,45,82,8,48,82,72,206,207,45,40,74,45,46,206,204,207,83,40,73,45,46,177,82,72,76,74,78,73,77,51,48,52,50,54,49,53,51,183,176,28,229,143,44,190,130,171,159,11,0,246,100,222,204,41,2,0,0};
static const trinity::u8 kZlib[]={120,156,11,41,202,204,203,44,169,84,112,42,202,47,47,78,45,82,8,48,82,72,206,207,45,40,74,45,46,206,204,207,83,40,73,45,46,177,82,72,76,74,78,73,77,51,48,52,50,54,49,53,51,183,176,28,229,143,44,190,130,171,159,11,0,241,207,154,221};
static const trinity::u8 kRaw[]={11,41,202,204,203,44,169,84,112,42,202,47,47,78,45,82,8,48,82,72,206,207,45,40,74,45,46,206,204,207,83,40,73,45,46,177,82,72,76,74,78,73,77,51,48,52,50,54,49,53,51,183,176,28,229,143,44,190,130,171,159,11,0};
static const char kExpected[]="Trinity Browser P2 compression test: abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789 END";

static bool run(const char* enc,const u8* src,usize n){u8 out[2048]{};usize got=0;const auto st=web::decode_http_content(enc,src,n,out,sizeof(out),got);return st==web::ContentDecodeStatus::Ok&&got==std::strlen(kExpected)&&std::memcmp(out,kExpected,got)==0;}
int main(){
 bool ok=true;ok=ok&&run("gzip",kGzip,sizeof(kGzip));ok=ok&&run("deflate",kZlib,sizeof(kZlib));ok=ok&&run("deflate",kRaw,sizeof(kRaw));
 u8 out[16]{};usize got=0;ok=ok&&web::decode_http_content("br",kGzip,sizeof(kGzip),out,sizeof(out),got)==web::ContentDecodeStatus::UnsupportedEncoding;
 got=0;ok=ok&&web::decode_http_content("gzip",kGzip,sizeof(kGzip),out,sizeof(out),got)==web::ContentDecodeStatus::OutputTooLarge;
 std::printf("browser_compat_p2_decode: %s\n",ok?"PASS":"FAIL");return ok?0:1;
}
