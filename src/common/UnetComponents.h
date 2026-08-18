// #include "../../libs/HEAAN/src/HEAAN.h"
#include "/home/csgrad/susimmuk/boxtraining/AdaFace/fhe_cpp/ImprovedLT/HEAAN/src/HEAAN.h"

using namespace std;
using namespace heaan;
using namespace NTL;

class UnetComponents
{
public:
    void relu(
        Ciphertext a,
        int sizeOfCiph,
        Ciphertext &reluRes,
        heaan::SecretKey &secretKey,
        heaan::Scheme &scheme, long n, long logp, 
        long logq);
};