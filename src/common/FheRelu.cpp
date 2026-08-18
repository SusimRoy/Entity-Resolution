// #include "../../libs/HEAAN/src/HEAAN.h"
#include "/home/csgrad/susimmuk/boxtraining/AdaFace/fhe_cpp/ImprovedLT/HEAAN/src/HEAAN.h"
#include "FheRelu.h"
#include "FheComp.h"

// using namespace heaan;
using namespace NTL;
using namespace std;

void FheRelu::relu(
    Ciphertext a,
    Ciphertext &reluRes,
    double &reluTime,
    SecretKey &secretKey,
    Scheme &scheme, long logp,
    long logq,
    bool useNewCompG)
{
    double compValE;
    double mult;
    double timeE;

    clock_t t0 = clock();

    complex<double> zero;
    zero.real(0.0);
    zero.imag(0.0);

    Ciphertext cipherZero;
    cipherZero = scheme.encryptSingle(zero, logp, logq);

    Ciphertext cipherComp;

    complex<double> inputdres;
    inputdres = scheme.decryptSingle(secretKey, a);

    FheComp fheComp = FheComp();

    double multCount = 0.0;

    if (useNewCompG)
    {   // Later
        // fheComp.newCompG(
        //     a, cipherZero, cipherComp,
        //     scheme, logp, logq, timeE, 3, 2, 2, multCount,
        //     secretKey);
    }
    else
    {
        fheComp.newComp(
            a, cipherZero, cipherComp,
            scheme, logp, logq, timeE, 3, 4, mult,
            secretKey);
    }

    complex<double> compDRes;

    compDRes = scheme.decryptSingle(secretKey, cipherComp);

    cout << "Relu input" << inputdres.real() << endl;
    cout << "Relu comp val" << compDRes.real() << endl;

    cout << "Expected" << inputdres.real() * compDRes.real() << endl;

    reluRes = scheme.mult(cipherComp, a);

    // Ciphertext testa;
    // Ciphertext testb;
    // complex<double> ta;
    // ta.real(scheme.decryptSingle(secretKey, a).real());
    // ta.imag(0.0);
    // complex<double> tb;
    // tb.real(scheme.decryptSingle(secretKey, cipherComp).real());
    // tb.imag(0.0);

    // testa = scheme.encryptSingle(ta, logp, logq);
    // testb = scheme.encryptSingle(tb, logp, logq);
    // testRes = scheme.mult(testa, testb);

    clock_t t1 = clock();

    reluTime = double(t1 - t0) / CLOCKS_PER_SEC;

    cout << "Relu res" << scheme.decryptSingle(secretKey, reluRes).real() << endl;
}

void FheRelu::relu(
    Ciphertext a,
    int sizeOfCiph,
    Ciphertext &reluRes,
    SecretKey &secretKey,
    Scheme &scheme, long n, long logp,
    long logq)
{
    
    Ciphertext cipherZeroVector;

    Ciphertext cipherZero = a;

    cipherZero = scheme.encryptZeros(n, a.logp, a.logq);

    Ciphertext cipherComp;

    FheComp fheComp = FheComp();

    double timeE, multCount;

    fheComp.newCompG(
            a,  cipherZero, cipherComp,
            scheme, n, logp, logq, timeE, 3, 2, 2, multCount,
            secretKey);
    
    scheme.reScaleByAndEqual(cipherComp, logp);

    while(a.logq != cipherComp.logq){
        scheme.modDownByAndEqual(a, logp);
    }
    
    reluRes = scheme.mult(cipherComp, a);
    scheme.reScaleByAndEqual(reluRes, logp);
}