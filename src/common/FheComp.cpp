// #include "../../libs/HEAAN/src/HEAAN.h"
#include "/home/csgrad/susimmuk/boxtraining/AdaFace/fhe_cpp/ImprovedLT/HEAAN/src/HEAAN.h"
#include "FheComp.h"
#include "omp.h"

using namespace NTL;
using namespace std;

void computeTerm(
    SchemeAlgo &algo,
    Scheme &scheme,
    double constTerm,
    Ciphertext x,
    double logp,
    int raisedTo,
    vector<Ciphertext> &resultVec,
    int resultIdx)
{
  Ciphertext powX;
  Ciphertext result;

  powX = algo.power(x, logp, raisedTo);
  scheme.multByConstAndEqual(powX, constTerm, logp);
  scheme.reScaleByAndEqual(powX, logp);

  resultVec[resultIdx] = scheme.addConst(powX, 0.0, logp);
  // resultVec[resultIdx] = powX;
}

void FheComp::newComp(
    Ciphertext cipher1,
    Ciphertext cipher2,
    Ciphertext &result,
    Scheme &scheme,
    long logp,
    long logq,
    double &time,
    int n, int d,
    double &multCount,
    SecretKey secretKey)
{
  clock_t t2 = clock();

  Ciphertext czero;
  czero = scheme.encryptSingle(0.0, logp, logq);
  // double x = a-b,
  Ciphertext prevX;
  prevX = scheme.encryptSingle(0.0, logp, logq);

  // Subtraction: A-B //
  Ciphertext cipherSub, imRes;
  cipherSub = scheme.sub(cipher1, cipher2);

  complex<double> dres;

  complex<double> bef;

  multCount = 0.0;
  for (int i = 0; i < d; i++)
  {
    // TODO: Later
    // fnxManualE(cipherSub, imRes, scheme, logp, logq, n,
    //            multCount, secretKey);

    cipherSub = scheme.addConst(imRes, 0.0, logp);
  }

  result = scheme.addConst(cipherSub, 1.0, logp);
  scheme.multByConstAndEqual(result, 0.5, logp);

  clock_t t3 = clock();

  time = ((double)(t3 - t2)) / CLOCKS_PER_SEC;
}

void FheComp::fnxManualE(
    Ciphertext x,
    Ciphertext &result,
    Scheme &scheme,
    SecretKey secretKey,
    long n,
    long logp,
    long logq,
    int f_n,
    double &multCount)
{
  SchemeAlgo algo(scheme);
  Ciphertext powX;

  double *toEncrypt = new double[n];
  double encrpytConstRes;

  if (f_n == 0)
  {
    encrpytConstRes = 1;
  }
  else
  {
    encrpytConstRes = 0;
  }

  for (size_t i = 0; i < n; ++i)
  {
    toEncrypt[i] = encrpytConstRes;
  }

  result = scheme.encrypt(toEncrypt, n, logp, logq);

  multCount += (n * (n - 1) + n);

  if (f_n == 0)
  {
    return;
  }
  else if (f_n == 1)
  {

    vector<double> constTerms = {-0.5, 1.5};
    int constCount = 1;

    for (int i = 1; i <= 3; i += 2)
    {
      powX = algo.power(x, logp, i);

      scheme.multByConstAndEqual(powX, constTerms[constCount--], logp);
      scheme.reScaleByAndEqual(powX, logp);

      scheme.modDownToAndEqual(result, powX.logq);
      scheme.addAndEqual(result, powX);
    }

    // cout << "Fnx_res" << scheme.decryptSingle(secretKey, result).real() << endl;
  }
  else if (f_n == 2)
  {

    vector<double> constTerms = {0.375, -1.25, 1.875};
    int constCount = 2;

    for (int i = 1; i <= 5; i += 2)
    {
      powX = algo.power(x, logp, i);

      scheme.multByConstAndEqual(powX, constTerms[constCount--], logp);
      scheme.reScaleByAndEqual(powX, logp);

      scheme.modDownToAndEqual(result, powX.logq);
      scheme.addAndEqual(result, powX);
    }
  }
  else if (f_n == 3)
  {

    vector<double> constTerms = {-0.3125, 1.3125, -2.1875, 2.1875};
    int constCount = 3;

    for (int i = 1; i <= 7; i += 2)
    {
      powX = algo.power(x, logp, i);

      scheme.multByConstAndEqual(powX, constTerms[constCount--], logp);

      scheme.reScaleByAndEqual(powX, logp);

      scheme.modDownToAndEqual(result, powX.logq);

      scheme.addAndEqual(result, powX);
    }
  }
  else if (f_n == 4)
  {
    // cout << "Using fnx for n=4\n";
    // cout << "Before fnx " << scheme.decryptSingle(secretKey, x).real() << endl;
    // --- IGNORE ---
    vector<double> constTerms = {0.2734375, -1.40625, 2.953125, -3.28125, 2.4609375};
    int constCount = 0;

    double res = 0.0;
    vector<Ciphertext> resultArr;

    auto resArr_i = 0;

    auto iter = 0;

    for (int i = 9; i >= 1; i -= 2)
    {
      // cout << "result " << scheme.decryptSingle(secretKey, result).real()<< endl;
      powX = algo.power(x, logp, i);
      // cout << "PowX after powering " << scheme.decryptSingle(secretKey, powX).real() << endl;

      double constT = constTerms[constCount++];

      auto exp = scheme.decryptSingle(secretKey, powX).real() * constT;

      res += exp;

      // cout << "PowX before multiplying the const "<<scheme.decryptSingle(secretKey, powX).real() <<endl;
      scheme.multByConstAndEqual(powX, constT, logp);

      auto bef_powlp = powX.logp;
      auto bef_powlq = powX.logq;

      auto bef_reslp = result.logp;
      auto bef_reslq = result.logq;

      // cout << "Before:" << endl;
      // cout << "powX logp = " << powX.logp << endl;
      // cout << "powX logq = " << powX.logq << endl;
      // cout << "Result logp = " << result.logp << endl;
      // cout << "Result logq = " << result.logq << endl;

      // cout << "PowX after multiplying the const "<<scheme.decryptSingle(secretKey, powX).real() <<endl;
      scheme.reScaleByAndEqual(powX, logp);

      // cout << "PowX after rescaling  "<<scheme.decryptSingle(secretKey, powX).real() <<endl;

      // cout << "Curr fnx term " << scheme.decryptSingle(secretKey, powX).real() << endl;
      if (iter == 0)
      {
        scheme.modDownToAndEqual(result, powX.logq);
      }
      else
      {
        scheme.modDownToAndEqual(powX, result.logq);
      }

      auto aft_powlp = powX.logp;
      auto aft_powlq = powX.logq;

      auto aft_reslp = result.logp;
      auto aft_reslq = result.logq;

      // cout << iter << ", " << bef_powlp << ", " << bef_powlq << ", " << bef_reslp << ", " << bef_reslq << ", " << aft_powlp << ", " << aft_powlq << ", " << aft_reslp << ", " << aft_reslq << ", "<<endl;

      // cout << "After:" << endl;

      // cout << "powX logp = " << powX.logp << endl;
      // cout << "powX logq = " << powX.logq << endl;
      // cout << "Result logp = " << result.logp << endl;
      // cout << "Result logq = " << result.logq << endl;

      scheme.addAndEqual(result, powX);

      // cout << "exp res" << res << endl;
      // cout << "curr res " << scheme.decryptSingle(secretKey, result).real() << endl;
      iter++;
    }
  }
  else
  {
    cout << "ERROR: fnx for n > 4 not supported\n";
  }
}

void FheComp::gnxManualE(
    Ciphertext x,
    Ciphertext &result,
    Scheme &scheme,
    SecretKey secretKey,
    long n,
    long logp,
    long logq,
    double f_n,
    double &multCount)
{

  SchemeAlgo algo(scheme);
  Ciphertext powX;
  complex<double> dres;

  double *toEncrypt = new double[n];
  double encrpytConstRes;

  if (f_n == 0)
  {
    encrpytConstRes = 1;
  }
  else
  {
    encrpytConstRes = 0;
  }

  for (size_t i = 0; i < n; ++i)
  {
    toEncrypt[i] = encrpytConstRes;
  }

  result = scheme.encrypt(toEncrypt, n, logp, logq);

  if (f_n == 0)
  {
    return;
  }
  else if (f_n == 1)
  {
    vector<double> constTerms = {-1359.0 / 1024, 2126.0 / 1024};
    int constCount = 1;

    for (int i = 1; i <= 3; i += 2)
    {
      powX = algo.power(x, logp, i);

      scheme.multByConstAndEqual(powX, constTerms[constCount--], logp);
      scheme.reScaleByAndEqual(powX, logp);

      scheme.modDownToAndEqual(result, powX.logq);
      scheme.addAndEqual(result, powX);
    }
  }
  else if (f_n == 2)
  {

    vector<double> constTerms = {3796.0 / 1024, -6108.0 / 1024, 3334.0 / 1024};
    int constCount = 2;

    for (int i = 1; i <= 5; i += 2)
    {
      powX = algo.power(x, logp, i);

      scheme.multByConstAndEqual(powX, constTerms[constCount--], logp);
      scheme.reScaleByAndEqual(powX, logp);

      scheme.modDownToAndEqual(result, powX.logq);
      scheme.addAndEqual(result, powX);
    }
  }
  else if (f_n == 3)
  {

    vector<double> constTerms = {-12860.0 / 1024, 25614.0 / 1024, -16577.0 / 1024, 4589.0 / 1024};

    int constCount = 3;

    for (int i = 1; i <= 7; i += 2)
    {
      powX = algo.power(x, logp, i);

      scheme.multByConstAndEqual(powX, constTerms[constCount--], logp);
      scheme.reScaleByAndEqual(powX, logp);

      scheme.modDownToAndEqual(result, powX.logq);
      scheme.addAndEqual(result, powX);

      // cout << "Term " << (i - 1) / 2 << ": " << scheme.decryptSingle(secretKey, powX).real() << endl;
      // cout << "power" << i <<endl;
      // for(int j=0; j<4; j++){
      //   cout << scheme.decrypt(secretKey, result)[j].real() <<endl;
      // }
    }
  }
  else if (f_n == 4)
  {

    vector<double> constTerms = {46623.0 / 1024, -113492.0 / 1024, 97015.0 / 1024, -34974.0 / 1024, 5850.0 / 1024};
    int constCount = 4;

    for (int i = 1; i <= 9; i += 2)
    {
      powX = algo.power(x, logp, i);

      scheme.multByConstAndEqual(powX, constTerms[constCount--], logp);
      scheme.reScaleByAndEqual(powX, logp);

      scheme.modDownToAndEqual(result, powX.logq);
      scheme.addAndEqual(result, powX);
    }
  }
  else
  {
    cout << "ERROR: fnx for n > 4 not supported\n";
  }
}

void FheComp::compGAgainstConst(
    Ciphertext cipher1,
    double constant,
    Ciphertext &result,
    Scheme &scheme,
    long n,
    long logp,
    long logq,
    int f_n, int dg, int df,
    SecretKey secretKey)
{
  Ciphertext cipherSub, imRes;
  
      cipherSub = scheme.addConst(cipher1, -constant, logp);

  complex<double> dres;
  double multCount;

  for (int i = 0; i < dg; i++)
  {
    gnxManualE(cipherSub, imRes,
               scheme, secretKey, n, logp, logq, f_n,
               multCount);

    cipherSub = scheme.addConst(imRes, 0.0, logp);
  }

  for (int i = 0; i < df; i++)
  {
    fnxManualE(cipherSub, imRes, scheme,
               secretKey, n, logp, logq, f_n, multCount);

    cipherSub = scheme.addConst(imRes, 0.0, logp);
  }

  scheme.addConstAndEqual(cipherSub, 1.0, logp);
  scheme.multByConstAndEqual(cipherSub, 0.5, logp);
  scheme.reScaleByAndEqual(cipherSub, logp);
  result = scheme.addConst(cipherSub, 0.0, logp);
}

void FheComp::newCompG(
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
    SecretKey secretKey)
{
  cout << "Entered for:" << f_n << "," << dg << "," << df << endl;

  clock_t t2 = clock();

  if (cipher1.logp != cipher2.logp || cipher2.logq != cipher2.logq)
  {
    throw std::runtime_error("Log p and q don't match");
  }

  // Subtraction: A-B //
  Ciphertext cipherSub, imRes;
  // cout << "Subtraction" << endl;
  cipherSub = scheme.sub(cipher1, cipher2);

  // cout << "sub" << scheme.decrypt(secretKey, cipherSub)[0].real() <<endl;

  complex<double> dres;
  // cout << "DG: " << dg << ", DF: " << df << endl;
  for (int i = 0; i < dg; i++)
  {
    // cout << "d = " << i << endl;
    // dres = scheme.decryptSingle(secretKey, cipherSub);

    gnxManualE(cipherSub, imRes,
               scheme, secretKey, n, logp, logq, f_n,
               multCount);

    cipherSub = scheme.addConst(imRes, 0.0, logp);
  }
  // cout << "After gnx" << scheme.decryptSingle(secretKey, cipherSub).real() << endl;
  for (int i = 0; i < df; i++)
  {
    // cout << "d = " << i << endl;
    // dres = scheme.decryptSingle(secretKey, cipherSub);

    fnxManualE(cipherSub, imRes, scheme,
               secretKey, n, logp, logq, f_n, multCount);

    // cout << "iter " << i << "fnx " << scheme.decryptSingle(secretKey, imRes).real() << endl;

    cipherSub = scheme.addConst(imRes, 0.0, logp);

    // cout << "iter " << i << "fnx " << scheme.decryptSingle(secretKey, cipherSub).real() << endl;
    //  cipherSub = imRes;
    //  dres = scheme.decryptSingle(secretKey, cipherSub);
    //  cout << scheme.decryptSingle(secretKey, cipherSub).real() << endl;
  }
  // cout << "After fnx " << scheme.decryptSingle(secretKey, cipherSub).real() << endl;
  // cout << "Post fnx " << scheme.decryptSingle(secretKey, cipherSub).real() << endl;

  // complex<double> one_by_two;
  // one_by_two.real(0.5);
  // one_by_two.imag(0);

  scheme.addConstAndEqual(cipherSub, 1.0, logp);
  scheme.multByConstAndEqual(cipherSub, 0.5, logp);

  result = scheme.addConst(cipherSub, 0.0, logp);

  clock_t t3 = clock();

  time = ((double)(t3 - t2)) / CLOCKS_PER_SEC;
}

void FheComp::scoreNormalize(
    vector<Ciphertext> &scores,
    vector<double> min,
    vector<double> max,
    vector<Ciphertext> &normScores,
    Ciphertext &fusedScore,
    double &timeTaken,
    Scheme &scheme,
    long n,
    long logp,
    long logq,
    SecretKey secretKey)
{
  SchemeAlgo algo(scheme);

  clock_t t0 = clock();

  double *vals = new double[1]{0.0};

  fusedScore = scheme.encrypt(vals, n, logp, logq);

  for (int i = 0; i < scores.size(); i++)
  {
    scheme.addConstAndEqual(scores[i], -min[i], logp);
    double denom = max[i] - min[i];

    Ciphertext currentNormScore;

    currentNormScore = scheme.multByConst(scores[i], (1.0 / denom), logp);
    normScores.push_back(currentNormScore);

    scheme.reScaleByAndEqual(currentNormScore, logp);
    scheme.modDownToAndEqual(fusedScore, currentNormScore.logq);

    scheme.addAndEqual(fusedScore, currentNormScore);
  }
  cout << endl;

  scheme.multByConstAndEqual(fusedScore, (1.0 / scores.size()), logp);

  // cout << "Average = " << scheme.decryptSingle(secretKey, averageScore).real() << endl;

  // Ciphertext zeroPointFive;
  // zeroPointFive = scheme.encryptSingle(0.5, logp, logq);

  double compTime;

  // scheme.reScaleByAndEqual(averageScore, logp);
  // scheme.modDownToAndEqual(averageScore, zeroPointFive.logq);

  double multDepth = 0;

  clock_t t1 = clock();

  timeTaken = ((double)(t1 - t0)) / CLOCKS_PER_SEC;
}

void FheComp::scoreNormalize(
    Ciphertext &cScores,
    double min,
    double max,
    Ciphertext &normScores,
    Scheme &scheme,
    long n,
    long logp,
    long logq,
    SecretKey secretKey)
{

  double one_by_denom = 1.0 / (max - min);

  normScores = scheme.addConst(cScores, -1 * min, logp);
  scheme.multByConstAndEqual(normScores, one_by_denom, logp);
}

void FheComp::standardScalar(
    Ciphertext &modIScores,
    double mean,
    double variance,
    Scheme &scheme,
    long n,
    long logp,
    long logq)
{
  if (std::abs(variance) < 1e-6)
  {
    scheme.addConstAndEqual(modIScores, 0.0, logp);
  }
  else
  {
    scheme.addConstAndEqual(modIScores, -mean, logp);
    scheme.multByConstAndEqual(modIScores, 1.0 / variance, logp);
    scheme.reScaleByAndEqual(modIScores, logp);
  }
}