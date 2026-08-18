// #include "../../libs/HEAAN/src/HEAAN.h"
#include "/home/csgrad/susimmuk/boxtraining/AdaFace/fhe_cpp/ImprovedLT/HEAAN/src/HEAAN.h"
using namespace std;
// using namespace heaan;
using namespace NTL;

class FheMathCore
{
public:
    void inverse(
        Ciphertext input, Scheme &scheme,
        Ciphertext &result, long logp, int iter,
        SecretKey SecretKey);
    
    void getMaxIdx(vector<Ciphertext> &ciphVec,
                   Ciphertext &maxIdx, Scheme &scheme,
                   SchemeAlgo &algo,
                   long n,
                   long logp,
                   long logq,
                   SecretKey secretKey);
    
    void getQPLC(
        Ciphertext &modIScores,
        vector<double> genQuantiles,
        vector<double> impQuantiles,
        Scheme &scheme,
        SchemeAlgo &algo,
        long n,
        long logp,
        long logq,
        SecretKey secretKey);
    
};
