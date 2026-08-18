// #include "../../libs/HEAAN/src/HEAAN.h"
#include "/home/csgrad/susimmuk/boxtraining/AdaFace/fhe_cpp/ImprovedLT/HEAAN/src/HEAAN.h"

using namespace std;
// using namespace heaan;
using namespace NTL;

class FheComp
{
public:
    void newComp(
        Ciphertext cipher1,
        Ciphertext cipher2,
        Ciphertext &result,
        Scheme &scheme,
        long logp,
        long logq,
        double &time,
        int n, int d,
        double &multCount,
        SecretKey secretKey);

    void newCompG(
        Ciphertext cipher1,
        Ciphertext cipher2,
        Ciphertext &result,
        Scheme &scheme,
        long n,
        long logp,
        long logq,
        double &time,
        int f_n, int dg, int df,
        double &multCount,
        SecretKey secretKey);

    void compGAgainstConst(
        Ciphertext cipher1,
        double constant,
        Ciphertext &result,
        Scheme &scheme,
        long n,
        long logp,
        long logq,
        int f_n, int dg, int df,
        SecretKey secretKey);


    void scoreNormalize(vector<Ciphertext> &scores,
                        vector<double> min,
                        vector<double> max,
                        vector<Ciphertext> &normScores,
                        Ciphertext &fusedScore,
                        double &timeTaken,
                        Scheme &scheme,
                        long n,
                        long logp,
                        long logq,
                        SecretKey secretKey);

    void scoreNormalize(Ciphertext &cScores,
                        double min,
                        double max,
                        Ciphertext &normScores,
                        Scheme &scheme,
                        long n,
                        long logp,
                        long logq,
                        SecretKey secretKey);

    void standardScalar(Ciphertext &modIScores,
                        double mean,
                        double variance,
                        Scheme &scheme,
                        long n,
                        long logp,
                        long logq);

    
    // void getCompsForQuantiles(
    //     vector<Ciphertext> scores,
    //     vector
    //     );

private:
    void fnxManualE(
        Ciphertext x,
        Ciphertext &result,
        Scheme &scheme,
        SecretKey secretKey,
        long n,
        long logp,
        long logq,
        int f_n,
        double &multCount);
    void gnxManualE(
        Ciphertext x,
        Ciphertext &result,
        Scheme &scheme,
        SecretKey secretKey,
        long n,
        long logp,
        long logq,
        double f_n,
        double &multCount);
};