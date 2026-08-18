// #include "../../libs/HEAAN/src/HEAAN.h"
#include "/home/csgrad/susimmuk/boxtraining/AdaFace/fhe_cpp/ImprovedLT/HEAAN/src/HEAAN.h"

using namespace std;
// using namespace heaan;
using namespace NTL;

class FheRelu
{
public:
    void relu(
        Ciphertext a,
        Ciphertext &reluRes,
        double &reluTime,
        SecretKey &secretKey,
        Scheme &scheme, long logp, 
        long logq,
        bool useNewCompG);
    void relu(
        Ciphertext a,
        int sizeOfCiph,
        Ciphertext &reluRes,
        SecretKey &secretKey,
        Scheme &scheme, long n, long logp, 
        long logq);
};