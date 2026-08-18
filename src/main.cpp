
#include <iterator>
#include <fstream>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <sys/resource.h>
#include <unordered_set>
#include <unordered_map>
#include "./common/FheComp.h"
#include "./common/FheRelu.h"
#include "./common/FheMathCore.h"
#include <omp.h>
#include <cstdlib>
#include <NTL/xdouble.h>
#include <NTL/ZZ.h>
#include "NTL/RR.h"
#include <NTL/ZZX.h>
#include "NTL/mat_RR.h"
#include "NTL/vec_RR.h"
#include <thread>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <random>
#include <vector>


using namespace std::chrono;
using namespace std;
// using namespace heaan;
using namespace NTL;

vector<vector<int>> dp_ncr(20, vector<int>(20, -1));
vector<vector<int>> dp_pow(20, vector<int>(20, -1));

// Parameters
long logq = 800; ///< Ciphertext modulus (this value should be <= logQ in "scr/Params.h")
long logp = 30;   ///< Scaling Factor (larger logp will give you more accurate value)
long logn = 15;   ///< number of slot is 1024 (this value should be < logN in "src/Params.h")
long n = 1 << 14;
long slots = n;
long numThread = 128;

int fib(int inp)
{
    if (inp == 0)
    {
        return 0;
    }
    if (inp == 1)
    {
        return 1;
    }

    int res = fib(inp - 1) + fib(inp - 2);

    return res;
}

double nCr(int n, int r)
{
    if (dp_ncr[n][r] != -1)
    {
        return dp_ncr[n][r];
    }
    if (r > n - r)
        r = n - r; // because C(n, r) == C(n, n - r)
    double ans = 1;
    int i;

    for (i = 1; i <= r; i++)
    {
        ans *= n - r + i;
        ans /= i;
    }
    dp_ncr[n][r] = ans;
    return ans;
}

// Has problems for n> 4, 5
double fn_x(double x, int n, double &multCount)
{
    double res = 0.0;

    for (int i = 0; i <= n; i++)
    {
        double currTerm = pow(0.25, i) * nCr(2 * i, i) * x * pow((double)1 - pow(x, 2), i); // pow = O(logy)
        multCount += (i > 0 ? (3 + floor(log2(i))) : 3);
        res += currTerm;
    }
    cout << "Res Final " << res << endl;
    // printf("Result %lf",res);
    // cout<<endl<<(res<-1.0)<<endl;
    return res;
}

void fnx_omp(double x, int n, double &multCount, double &result)
{
#pragma omp parallel for reduction(+ : result)
    for (int i = 0; i <= n; i++)
    {
        double currTerm = pow(0.25, i) * nCr(2 * i, i) * x * pow((double)1 - pow(x, 2), i); // pow = O(logy)
        // multCount += (i > 0 ? (3 + floor(log2(i))) : 3);
        result += currTerm;
    }

    cout << "exit fnx" << endl;
    // printf("Result %lf",res);
    // cout<<endl<<(res<-1.0)<<endl;
}

void gnx_manual(double x, int n, double &result)
{
    vector<double> constTerms;

    if (n == 0)
    {
    }
    else if (n == 1)
    {
        auto denom = pow(2, 10);
        constTerms = {-1359, 2126};

        for (int ci = 0; ci < constTerms.size(); ci++)
        {
            constTerms[ci] /= denom;
        }
    }
    else if (n == 2)
    {
        auto denom = pow(2, 10);
        constTerms = {3796, -6108, 3334};

        for (int ci = 0; ci < constTerms.size(); ci++)
        {
            constTerms[ci] /= denom;
        }
    }
    else if (n == 3)
    {
        auto denom = pow(2, 10);
        constTerms = {-12860, 25614, -16577, 4589};

        for (int ci = 0; ci < constTerms.size(); ci++)
        {
            constTerms[ci] /= denom;
        }
    }
    else if (n == 4)
    {
        auto denom = pow(2, 10);
        constTerms = {46623, -113492, 97015, -34974, 5850};

        for (int ci = 0; ci < constTerms.size(); ci++)
        {
            constTerms[ci] /= denom;
        }
    }

    auto x_pow = 2 * n + 1;

    for (int ci = 0; ci < constTerms.size(); ci++)
    {
        result += (constTerms[ci] * pow(x, x_pow));
        x_pow -= 2;
    }
    cout << "res ser " << result << endl;
}

void gnx_omp(double x, int n, double &result)
{
    vector<double> constTerms;

    if (n == 0)
    {
    }
    else if (n == 1)
    {
        auto denom = pow(2, 10);
        constTerms = {-1359, 2126};

        for (int ci = 0; ci < constTerms.size(); ci++)
        {
            constTerms[ci] /= denom;
        }
    }
    else if (n == 2)
    {
        auto denom = pow(2, 10);
        constTerms = {3796, -6108, 3334};

        for (int ci = 0; ci < constTerms.size(); ci++)
        {
            constTerms[ci] /= denom;
        }
    }
    else if (n == 3)
    {
        auto denom = pow(2, 10);
        constTerms = {-12860, 25614, -16577, 4589};

        for (int ci = 0; ci < constTerms.size(); ci++)
        {
            constTerms[ci] /= denom;
        }
    }
    else if (n == 4)
    {
        auto denom = pow(2, 10);
        constTerms = {46623, -113492, 97015, -34974, 5850};

        for (int ci = 0; ci < constTerms.size(); ci++)
        {
            constTerms[ci] /= denom;
        }
    }

    auto x_pow = 2 * n + 1;

#pragma omp parallel
    for (int ci = 0; ci < constTerms.size(); ci++)
    {
        result += (constTerms[ci] * pow(x, x_pow));
        x_pow -= 2;
    }
    cout << "result par " << result << endl;
}

double newcomp(double a, double b, int n, int d, double &multCount)
{
    double x = a - b, prevX = 0.0;
    multCount = 0.0;

    for (int i = 0; i < d; i++)
    {
        x = fn_x(x, n, multCount);
        // cout<<"\nMultiplication count:"<< multCount<<endl;
        if (isnan(x))
        {
            std::cout << "ERROR: x is out of range. Check comparison function!" << endl;
            exit(0);
        }
        prevX = x;
    }
    return (x + 1.0) / 2.0;
}

double newCompG(double a, double b, int n, int dg, int df)
{
    auto x = a - b;
    auto result = 0.0;

    for (int i = 0; i < dg; i++)
    {
        result = 0.0;
        gnx_manual(x, n, result);
        x = result;
        cout << "ser x = " << x << endl;
    }

    cout << "Post gnx " << result << endl;

    auto multCount = 0.0;

    for (int i = 0; i < df; i++)
    {
        result = fn_x(result, n, multCount);
    }

    cout << "Post fnx " << result << endl;

    return (result + 1.0) / 2.0;
}

double newCompG_omp(double a, double b, int n, int dg, int df)
{
    auto x = a - b;
    auto result = 0.0;

    for (int i = 0; i < dg; i++)
    {
        result = 0.0;
        gnx_omp(x, n, result);
        x = result;
        cout << "par x = " << x << endl;
    }

    auto multCount = 0.0;

    for (int i = 0; i < df; i++)
    {
        result = 0.0;
        fnx_omp(x, n, multCount, result);
        x = result;
    }

    return (x + 1.0) / 2.0;
}
/*
Run ./main with params
1 -> newComp
2 -> newCompG
3 -> both
*/
int main(int argc, char **argv)
{
    /*
     * Basic Parameters are in src/Params.h
     * If you want to use another parameter, you need to change src/Params.h file and re-complie this library.
     */

    ofstream myfile;
    // Format: n, d, a, b, time_unenc, comp_val_unenc, time_enc, comp_val_enc
    myfile.open("./data/new_comp.csv", ios::app);

    string run_flag = "37";

    vector<double> candidateN(4);
    iota(candidateN.begin(), candidateN.end(), 1);

    vector<double> candidateD(4);
    iota(candidateD.begin(), candidateD.end(), 1);

    TimeUtils timeutils;
    long logN = logn;
    long logQ = logq;
    double sigma = 3.2;
    long h = 64;
    Ring ring(logN, logQ);
    SecretKey secretKey(ring);
    Scheme scheme(secretKey, ring);
    SchemeAlgo algo(scheme);
    srand(time(NULL));
    SetNumThreads(numThread);
    scheme.addLeftRotKeys(secretKey);  ///< When you need left rotation for the vectorized message
    scheme.addRightRotKeys(secretKey); ///< When you need right rotation for the vectorized message

    vector<pair<double, double>> ABPairs;

    FheComp fheComp;
    FheRelu fheRelu;
    FheMathCore fheMathCore;

    for (auto a = 0.04; a <= 1; a = a + 0.04)
    {
        std::pair<double, double> p(a, 0.52);
        ABPairs.push_back(p);
    }

    if (run_flag == "1" || run_flag == "3")
    {
        cout << "Running NewComp" << endl;
        for (int i = 0; i < ABPairs.size(); i++)
        {
            pair<double, double> p = ABPairs[i];
            complex<double> temp1;
            temp1.real(p.first);
            temp1.imag(0);
            complex<double> temp2;
            temp2.real(p.second);
            temp2.imag(0);
            // Encrypt Two Arry of Complex //
            Ciphertext cipher1;
            cipher1 = scheme.encryptSingle(temp1, logp, logq);
            Ciphertext cipher2;
            cipher2 = scheme.encryptSingle(temp2, logp, logq);

            double abs_err = 0.0, modelCount = 10.0;

            for (int candn = 0; candn < candidateN.size(); candn++)
            {
                for (int candd = 0; candd < candidateD.size(); candd++)
                {
                    int n = candidateN[candn];
                    int d = candidateD[candd];

                    double multCount = 0.0;
                    double multCountE = 0.0;

                    double comp_valueE;
                    clock_t t0 = clock();
                    double comp_value = newcomp(temp1.real(), temp2.real(), n, d, multCount);

                    clock_t t1 = clock();

                    cout << "a= " << temp1.real() << " b= " << temp2.real() << " n= " << n << " d= " << d << endl;

                    double time;
                    Ciphertext result;

                    fheComp.newComp(cipher1, cipher2, result,
                                    scheme, logp, logq, time, n, d, multCountE, secretKey);

                    complex<double> dnum1 = scheme.decryptSingle(secretKey, cipher1);
                    complex<double> dnum2 = scheme.decryptSingle(secretKey, cipher2);

                    std::cout << "\nNum1 (a): " << dnum1 << endl;
                    std::cout << "\nNum2 (b):" << dnum2 << endl;

                    myfile << n << ", " << d << ", ";

                    myfile << temp1.real() << ", " << temp2.real() << ", ";

                    myfile << ((double)(t1 - t0)) / CLOCKS_PER_SEC << ", ";

                    myfile << comp_value << ", " << multCount << ", ";

                    myfile << time << ", " << scheme.decryptSingle(secretKey, result).real()
                           << ", " << multCountE << ", " << endl;
                }
            }
        }
    }

    myfile.close();

    // NewCompG
    ofstream myfile2;
    // Format: n, d, a, b, time_unenc, comp_val_unenc, time_enc, comp_val_enc
    myfile2.open("./data/new_compG.csv", ios::app);

    if (run_flag == "2" || run_flag == "3")
    {
        cout << "Running NewCompG" << endl;
        for (int i = 0; i < ABPairs.size(); i++)
        {
            pair<double, double> p = ABPairs[i];
            complex<double> temp1;
            temp1.real(p.first);
            temp1.imag(0);
            complex<double> temp2;
            temp2.real(p.second);
            temp2.imag(0);
            // Encrypt Two Arry of Complex //
            Ciphertext cipher1;
            cipher1 = scheme.encryptSingle(temp1, logp, logq);
            Ciphertext cipher2;
            cipher2 = scheme.encryptSingle(temp2, logp, logq);

            double abs_err = 0.0, modelCount = 10.0;

            for (int candn = 0; candn < candidateN.size(); candn++)
            {
                for (int dg = 1; dg <= 4; dg++)
                {
                    for (int df = 1; df < 6 - dg; df++)
                    {
                        if (dg == 1 && df == 4)
                        {
                            continue;
                        }

                        if (dg == 2 && df == 3)
                        {
                            continue;
                        }

                        if (dg == 3 && df == 2)
                        {
                            continue;
                        }

                        if (dg == 4 && df == 1)
                        {
                            continue;
                        }

                        int n = candidateN[candn];

                        // double multCount = 0.0;
                        // double multCountE = 0.0;

                        double comp_valueE;
                        clock_t t0 = clock();
                        double comp_value = newCompG(temp1.real(), temp2.real(), n, dg, df);

                        clock_t t1 = clock();

                        cout << "a= " << temp1.real() << " b= " << temp2.real() << " n= " << n << " dg= " << dg << " df= " << df << endl;

                        double time;
                        Ciphertext result;
                        double mult;

                        // fheComp.newCompG(cipher1, cipher2, result,
                        //                  scheme, logp, logq, time, n, dg, df, mult,
                        //                  secretKey);

                        complex<double> dnum1 = scheme.decryptSingle(secretKey, cipher1);
                        complex<double> dnum2 = scheme.decryptSingle(secretKey, cipher2);

                        std::cout << "\nNum1 (a): " << dnum1 << endl;
                        std::cout << "\nNum2 (b):" << dnum2 << endl;

                        myfile2 << n << ", " << dg << ", " << df << ", ";

                        myfile2 << temp1.real() << ", " << temp2.real() << ", ";

                        myfile2 << ((double)(t1 - t0)) / CLOCKS_PER_SEC << ", ";

                        myfile2 << comp_value << ", ";

                        myfile2 << time << ", " << scheme.decryptSingle(secretKey, result).real()
                                << ", " << endl;
                    }
                }
            }
        }
    }

    myfile2.close();

    if (run_flag == "8")
    {

        cout << "test debug 8" << endl;

        double time;
        Ciphertext result;
        double mult;

        complex<double> temp1;
        temp1.real(0.1);
        temp1.imag(0);
        complex<double> temp2;
        temp2.real(0.7);
        temp2.imag(0);
        // Encrypt Two Arry of Complex //
        Ciphertext cipher1;
        cipher1 = scheme.encryptSingle(temp1, logp, logq);
        Ciphertext cipher2;
        cipher2 = scheme.encryptSingle(temp2, logp, logq);

        fheComp.newComp(
            cipher1, cipher2, result,
            scheme, logp, logq, time, 4, 4, mult,
            secretKey);

        cout << scheme.decryptSingle(secretKey, result).real() << endl;
        cout << "Serial ex " << time << endl;

        // Ciphertext parRes;
        // double parTime;
        // double multPar;
        // fheComp.newComp(
        //     cipher1, cipher2, parRes,
        //     scheme, logp, logq, parTime, 3, 4, multPar,
        //     secretKey, true);

        // cout << scheme.decryptSingle(secretKey, parRes).real() << endl;
        // cout << "Parallel ex " << parTime << endl;
    }

    if (run_flag == "9")
    {
        double time;
        Ciphertext result;

        complex<double> temp1;
        temp1.real(0.8);
        temp1.imag(0);
        complex<double> temp2;
        temp2.real(0.2);
        temp2.imag(0);
        // Encrypt Two Arry of Complex //
        Ciphertext cipher1;
        cipher1 = scheme.encryptSingle(temp1, logp, logq);
        Ciphertext cipher2;
            cipher2 = scheme.encryptSingle(temp2, logp, logq);

        double comp_value = newCompG(temp1.real(), temp2.real(), 4, 2, 1);

        cout << "Comp Val Unenc" << comp_value << endl;

        double mult;

        // fheComp.newCompG(
        //     cipher1, cipher2, result,
        //     scheme, logp, logq, time, 4, 2, 1, mult,
        //     secretKey);

        cout << "Comp Val enc" << scheme.decryptSingle(secretKey, result).real() << endl;
        cout << "Serial ex newCompG " << time << endl;

        // Ciphertext parRes;
        // double parTime;
        // fheComp.newCompG(
        //     cipher1, cipher2, parRes,
        //     scheme, logp, logq, parTime, 4, 2, 2,
        //     secretKey, true);

        // cout << scheme.decryptSingle(secretKey, parRes).real() << endl;
        // cout << "Parallel ex newCompG " << parTime <<endl;
    }

    // Run basic comparison and compute the time
    // Use gdb profiler to check how long each of the operations takes for encrypted and unencrypted

    if (run_flag == "10")
    {
        double totalTime = 0;
        double a = 0.1;
        double b = 0.8;
        bool comp;
        for (int i = 0; i < 1000; i++)
        {
            clock_t t0 = clock();
            comp = a > b;
            clock_t t1 = clock();
            totalTime += double(t1 - t0) / CLOCKS_PER_SEC;
        }
        cout << "Average Time" << totalTime / 1000 << endl;
    }

    if (run_flag == "11")
    {

        ofstream reluFile;
        reluFile.open("./data/relu_res.csv", ios::app);

        vector<double> inputs;

        for (double i = -1.0; i <= 1.0; i = i + 0.04)
        {
            inputs.push_back(i);

            Ciphertext cipherReluRes1;
            Ciphertext cipherReluRes2;
            Ciphertext input;

            complex<double> inpComplex;
            inpComplex.real(i);
            inpComplex.imag(0);

            input = scheme.encryptSingle(inpComplex, logp, logq);

            double time1, time2;

            fheRelu.relu(
                input,
                cipherReluRes1,
                time1,
                secretKey,
                scheme, logp, logq,
                false);

            fheRelu.relu(
                input,
                cipherReluRes2,
                time2,
                secretKey,
                scheme, logp, logq,
                true);

            reluFile << inpComplex.real() << ", "
                     << scheme.decryptSingle(secretKey, cipherReluRes1).real() << ","
                     << time1 << ","
                     << scheme.decryptSingle(secretKey, cipherReluRes2).real() << ","
                     << time2 << ","
                     << endl;
        }

        reluFile.close();
    }

    if (run_flag == "12")
    {

        int n = 3;
        int d = 4;
        int n2 = 1;
        int dg = 2;
        int df = 2;

        ofstream fNewComp;
        fNewComp.open("./data/new_comp_" + to_string(n) + "_" + to_string(d) + ".csv", ios::app);

        ofstream fNewCompG;
        fNewCompG.open(
            "./data/new_compG_" + to_string(n2) + "_" + to_string(dg) + "_" + to_string(df) + ".csv",
            ios::app);

        for (int i = 0; i < ABPairs.size(); i++)
        {
            double multCount = 0.0;
            double multCountE = 0.0;

            pair<double, double> p = ABPairs[i];
            complex<double> temp1;
            temp1.real(p.first);
            temp1.imag(0);
            complex<double> temp2;
            temp2.real(p.second);
            temp2.imag(0);
            // Encrypt Two Arry of Complex //
            Ciphertext cipher1;
            cipher1 = scheme.encryptSingle(temp1, logp, logq);
            Ciphertext cipher2;
            cipher2 = scheme.encryptSingle(temp2, logp, logq);

            double comp_valueE;
            clock_t t0 = clock();
            double comp_value = newcomp(temp1.real(), temp2.real(), n, d, multCount);

            clock_t t1 = clock();

            cout << "a= " << temp1.real() << " b= " << temp2.real() << " n= " << n << " d= " << d << endl;

            double time;
            Ciphertext result;

            fheComp.newComp(cipher1, cipher2, result,
                            scheme, logp, logq, time, n, d, multCountE, secretKey);

            complex<double> dnum1 = scheme.decryptSingle(secretKey, cipher1);
            complex<double> dnum2 = scheme.decryptSingle(secretKey, cipher2);

            fNewComp << n << ", " << d << ", ";

            fNewComp << temp1.real() << ", " << temp2.real() << ", ";

            fNewComp << ((double)(t1 - t0)) / CLOCKS_PER_SEC << ", ";

            fNewComp << comp_value << ", " << multCount << ", ";

            fNewComp << time << ", " << scheme.decryptSingle(secretKey, result).real()
                     << ", " << multCountE << ", " << endl;

            // New Comp G

            t0 = clock();
            comp_value = newCompG(temp1.real(), temp2.real(), n, dg, df);

            t1 = clock();

            cout << "a= " << temp1.real() << " b= " << temp2.real() << " n= " << n2 << " dg= " << dg << " df= " << df << endl;

            double mult;

            // fheComp.newCompG(cipher1, cipher2, result,
            //                  scheme, logp, logq, time, n2, dg, df, mult,
            //                  secretKey);

            fNewCompG << n2 << ", " << dg << ", " << df << ", ";

            fNewCompG << temp1.real() << ", " << temp2.real() << ", ";

            fNewCompG << ((double)(t1 - t0)) / CLOCKS_PER_SEC << ", ";

            fNewCompG << comp_value << ", ";

            fNewCompG << time << ", " << scheme.decryptSingle(secretKey, result).real()
                      << ", " << endl;
        }

        fNewComp.close();
        fNewCompG.close();
    }

    if (run_flag == "13")
    {
        Ciphertext ct1;
        Ciphertext ct2;
        Ciphertext res;
        double cnst = 2.95312;

        ct1 = scheme.encryptSingle(0.451814, logp, logq);
        ct2 = scheme.encryptSingle(2.95312, logp, logq);
        scheme.multByConstAndEqual(ct1, cnst, logp);

        cout << "Res " << scheme.decryptSingle(secretKey, ct1).real() << endl;
    }

    if (run_flag == "14")
    {
        double time;
        double result;

        auto t0 = clock();
        double comp_value = newCompG(0.1, 0.7, 3, 2, 2);
        auto t1 = clock();
        cout << "Comp Val Unenc" << comp_value << endl;
        cout << "Serial ex newCompG " << ((double)(t1 - t0)) / CLOCKS_PER_SEC << endl;

        t0 = clock();
        comp_value = newCompG_omp(0.1, 0.7, 3, 2, 2);
        t1 = clock();
        cout << "Comp Val Unenc par" << comp_value << endl;
        cout << "Parallel ex newCompG " << ((double)(t1 - t0)) / CLOCKS_PER_SEC << endl;
    }

    if (run_flag == "15")
    {
        ofstream scoreNormFile;
        scoreNormFile.open("./data/score_norm.csv", ios::app);

        vector<Ciphertext> normResults;

        vector<double> scores = {45.0, 55.0, 65.0, 65.0, 750.0, 8.5, 0.9, 70.0, 4300.0};
        vector<double> min = {0.0, 0.0, 0.0, 10.0, 125.0, 0.5, 0.05, 0.0, 200.0};
        vector<double> max = {100.0, 100.0, 120.0, 100.0, 900.0, 10.0, 1.0, 100.0, 5000.0};

        double unencTime;

        clock_t t0 = clock();

        auto start = high_resolution_clock::now();

        double avg_score = 0.0;

        for (int i = 0; i < scores.size(); i++)
        {
            avg_score += (scores[i] - min[i]) / (max[i] - min[i]);
        }

        avg_score /= scores.size();

        clock_t t1 = clock();

        auto stop = high_resolution_clock::now();

        auto duration = duration_cast<microseconds>(stop - start);

        cout << duration.count() << endl;

        double unEncTime = ((double)(t1 - t0)) / CLOCKS_PER_SEC;

        scoreNormFile << "UnEnc Time = " << unEncTime << endl;

        vector<double> expRes;

        vector<Ciphertext> cScores;
        for (int i = 0; i < scores.size(); i++)
        {
            Ciphertext ct;
            ct = scheme.encryptSingle(scores[i], logp, logq);
            cScores.push_back(ct);

            expRes.push_back((scores[i] - min[i]) / (max[i] - min[i]));
        }

        cout << endl;

        double time;

        // fheComp.scoreNormalize(cScores, min, max, normResults, time,
        //                        scheme, logp, logq, secretKey);

        scoreNormFile << "Enc Time = " << time << endl
                      << endl;

        for (int i = 0; i < expRes.size(); i++)
        {
            scoreNormFile << scores[i] << ", " << min[i] << ", " << max[i] << ", " << expRes[i] << ", " << scheme.decryptSingle(secretKey, normResults[i]).real() << endl;
        }

        scoreNormFile.close();
    }

    if (run_flag == "16")
    {
        ofstream scoreNormFile;
        scoreNormFile.open("./data/score_norm_models.csv", ios::app);

        vector<Ciphertext> normResults;

        int num_models = 25;

        vector<double> scores = {45.0, 55.0, 65.0, 65.0, 750.0, 8.5, 0.9, 70.0, 4300.0, 66.0, 78.0, 65.0, 65.0, 870.0, 8.5, 0.98, 79.0, 4350.0, 49.0, 58.0, 65.0, 78.0, 430.0, 8.5, 0.91};
        vector<double> min = {0.0, 0.0, 0.0, 10.0, 125.0, 0.5, 0.05, 0.0, 200.0, 0.0, 0.0, 0.0, 10.0, 125.0, 0.5, 0.05, 0.0, 200.0, 0.0, 0.0, 0.0, 10.0, 125.0, 0.5, 0.05};
        vector<double> max = {100.0, 100.0, 120.0, 100.0, 900.0, 10.0, 1.0, 100.0, 5000.0, 100.0, 100.0, 120.0, 100.0, 900.0, 10.0, 1.0, 100.0, 5000.0, 100.0, 100.0, 120.0, 100.0, 900.0, 10.0, 1.0};

        double unencTime;

        clock_t t0 = clock();

        auto start = high_resolution_clock::now();

        double avg_score = 0.0;

        for (int i = 0; i < num_models; i++)
        {
            avg_score += (scores[i] - min[i]) / (max[i] - min[i]);
        }

        avg_score /= scores.size();

        clock_t t1 = clock();

        auto stop = high_resolution_clock::now();

        auto duration = duration_cast<microseconds>(stop - start);

        cout << duration.count() << endl;

        double unEncTime = ((double)(t1 - t0)) / CLOCKS_PER_SEC;

        scoreNormFile << "UnEnc Time = " << unEncTime << endl;

        vector<double> expRes;

        vector<Ciphertext> cScores;
        for (int i = 0; i < num_models; i++)
        {
            Ciphertext ct;
            ct = scheme.encryptSingle(scores[i], logp, logq);
            cScores.push_back(ct);

            expRes.push_back((scores[i] - min[i]) / (max[i] - min[i]));
        }

        cout << endl;

        double time;

        // fheComp.scoreNormalize(cScores, min, max, normResults, time,
        //                        scheme, logp, logq, secretKey);

        scoreNormFile << "Enc Time = " << time << endl
                      << endl;

        for (int i = 0; i < expRes.size(); i++)
        {
            scoreNormFile << scores[i] << ", " << min[i] << ", " << max[i] << ", " << expRes[i] << ", " << scheme.decryptSingle(secretKey, normResults[i]).real() << endl;
        }

        scoreNormFile.close();
    }

    if (run_flag == "17")
    {
        vector<double> decOuts = {1, 1, 0, 0, 1, 1, 1, 1, 1};
        vector<double> decOuts1 = {1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1};
        vector<double> decOuts2 = {0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

        Ciphertext decSum;
        vector<Ciphertext> ciphDecs;

        decSum = scheme.encryptSingle(0, logp, logq);

        for (int i = 0; i < decOuts2.size(); i++)
        {
            Ciphertext ct;
            complex<double> temp_val(decOuts2[i], 0);
            ct = scheme.encryptSingle(temp_val, logp, logq);
            ciphDecs.push_back(ct);
        }

        clock_t t2 = clock();

        for (int i = 0; i < ciphDecs.size(); i++)
        {
            scheme.addAndEqual(decSum, ciphDecs[i]);
        }

        cout << "added" << scheme.decryptSingle(secretKey, decSum).real() << endl;

        scheme.multByConstAndEqual(decSum, (1.0 / decOuts2.size()), logp);

        cout << "Scaled down " << scheme.decryptSingle(secretKey, decSum).real() << endl;

        double compTime = 0;

        clock_t t3 = clock();

        double time = ((double)(t3 - t2)) / CLOCKS_PER_SEC;

        cout << (time) << endl;

        complex<double> temp2;
        temp2.real(0.5);
        temp2.imag(0);
        // Encrypt Two Arry of Complex //
        Ciphertext cipher1;
        cipher1 = scheme.encryptSingle(temp2, logp, logq);

        Ciphertext res;

        double mult;

        // fheComp.newCompG(decSum, cipher1, res,
        //                                  scheme, logp, logq, compTime, 3, 2, 2, mult,
        //                                  secretKey);

        cout << "Total time " << time + compTime << endl;
    }

    // generate time graphs
    // Get Average score level fusion time
    // Get Average decision level fusion time
    // Get Unenc Average score level fusion time
    // Get Unenc Average decision level fusion time

    if (run_flag == "18")
    {
        // mult depth
        ofstream depthFile;
        depthFile.open("./data/depth.csv", ios::app);

        for (int n = 1; n <= 4; n++)
        {
            for (int d = 1; d <= 4; d++)
            {
                int depth = d * ((n + 1) * (n + 1));
                depthFile << n << ", " << d << ", " << depth << ", " << endl;
            }
        }

        depthFile.close();
    }

    if (run_flag == "19")
    {
        cout << "Check multiplicative depth" << endl;
        // check multiplicative depth
        complex<double> temp1;
        temp1.real(0.5);
        temp1.imag(0);

        Ciphertext x;
            x = scheme.encryptSingle(temp1, logp, logq);

        double unenc = 0.5;

        double ciphdec = 0.5;

        double multcount;

        while (abs(unenc - ciphdec) < 0.001)
        {
            multcount += 1;
            scheme.multAndEqual(x, x);
            ciphdec = scheme.decryptSingle(secretKey, x).real();

            unenc *= unenc;

            cout << multcount << endl;
        }
    }

    if (run_flag == "20")
    {
        Ciphertext vecCiph, reluRes;

        double *vals = new double[4]{-0.1, 0.2, 0.5, 0.8};

        vecCiph = scheme.encrypt(vals, n, logp, logq);

        fheRelu.relu(vecCiph, 4, reluRes, secretKey, scheme, n, logp, logq);

        for (int i = 0; i < 4; i++)
        {
            cout << scheme.decrypt(secretKey, reluRes)[i].real() << endl;
        }

        delete vals;
    }

    if (run_flag == "21")
    {
        std::ifstream file("./data/bssr1.csv");

        if (!file.is_open())
        {
            std::cout << "Failed to open the file." << std::endl;
            return -1;
        }

        std::vector<std::vector<std::string>> data, filteredData; // Store the CSV data as a 2D vector of strings
        std::unordered_set<string> set;

        std::string line;
        int lineno = 0;
        while (std::getline(file, line))
        {
            std::stringstream ss(line);
            std::vector<std::string> row;
            std::string cell;

            if (lineno == 0)
            {
                lineno++;
                continue;
            }

            int item = 0;
            string user_id_1, user_id_2, value;

            while (std::getline(ss, cell, ','))
            {
                if (item == 0)
                {
                    user_id_1 = cell;
                }

                row.push_back(cell);
                item++;
            }

            data.push_back(row);
            set.insert(user_id_1);
        }

        file.close();

        ofstream nist_fusion;
        nist_fusion.open("./data/nist_fusion_susim.csv", ios::app);

        for (const auto &usrId : set)
        {

            int matedCount = 0;
            int unmatedCount = 0;

            for (const auto &row : data)
            {
                if (matedCount > 0 && unmatedCount > 49)
                {
                    break;
                }

                if (row[0] != usrId)
                {
                    continue;
                }

                if (row[0] == row[1])
                {
                    filteredData.push_back(row);
                    matedCount++;
                }
                else if (unmatedCount < 5)
                {
                    filteredData.push_back(row);
                    unmatedCount++;
                }
            }
        }

        vector<Ciphertext> normRes;

        vector<double> scores, min, max;
        double unencAvg;
        vector<Ciphertext> cScores;
        double timeTaken;

        min.push_back(0);
        min.push_back(0);
        min.push_back(0);
        min.push_back(0);

        max.push_back(1);
        max.push_back(100);
        max.push_back(100);
        max.push_back(100);

        for (const auto &row : filteredData)
        {
            scores.clear();
            cScores.clear();
            normRes.clear();

            scores.push_back(stod(row[3]));
            scores.push_back(stod(row[4]));
            scores.push_back(stod(row[5]));
            scores.push_back(stod(row[6]));

            unencAvg = 0.0;

            for (int i = 0; i < scores.size(); i++)
            {

                unencAvg += (scores[i] - min[i]) / (max[i] - min[i]);

                Ciphertext ct;
                complex<double> cplx;
                cplx.real(scores[i]);
                cplx.imag(0.0);

                ct = scheme.encryptSingle(cplx, logp, logq);

                cScores.push_back(ct);
            }

            unencAvg /= scores.size();

            Ciphertext fusedScore;

            fheComp.scoreNormalize(cScores, min, max,
                                   normRes, fusedScore,
                                   timeTaken,
                                   scheme, n, logp, logq, secretKey);

            double fs = scheme.decrypt(secretKey, fusedScore)[0].real();

            Ciphertext zeroPointFive, compRes;
            double *vals = new double[n];
            for(long i = 0; i < n; i++) vals[i] = 0.5;
            for(long i = 0; i < n; i++) vals[i] = 0;
            zeroPointFive = scheme.encrypt(vals, n, logp, logq);
            delete[] vals;

            scheme.reScaleByAndEqual(fusedScore, logp);
            scheme.modDownToAndEqual(fusedScore, zeroPointFive.logq);

            double multCount;
            double unEncCompVal;

            if (unencAvg > 0.5)
            {
                unEncCompVal = 1;
            }
            else if (unencAvg == 0.5)
            {
                unEncCompVal = 0.5;
            }
            else
            {
                unEncCompVal = 0;
            }

            fheComp.newCompG(fusedScore,
                             zeroPointFive, compRes,
                             scheme, n, logp, logq,
                             timeTaken, 4, 2, 1, multCount, secretKey);
            nist_fusion << row[0] << ", " << scores[0] << ", " << scores[1] << ", " << scores[2] << ", " << scores[3] << ", "
                        << unencAvg << ", " << unEncCompVal << ", "
                        << fs << ", " << scheme.decrypt(secretKey, compRes)[0].real() << endl;
        }

        nist_fusion.close();
    }

    if (run_flag == "22")
    {
        std::ifstream file("./data/nist_fusion.csv");

        int correct_count = 0;

        if (!file.is_open())
        {
            std::cout << "Failed to open the file." << std::endl;
            return -1;
        }

        std::vector<std::vector<std::string>> data;

        std::string line;
        while (std::getline(file, line))
        {
            std::stringstream ss(line);
            std::vector<std::string> row;
            std::string cell;

            while (std::getline(ss, cell, ','))
            {
                row.push_back(cell);
            }

            int enc_dec = -1;
            if (stod(row[8]) > 0.5)
            {
                enc_dec = 1;
            }
            else
            {
                enc_dec = 0;
            }

            if (abs(stod(row[6]) - enc_dec) == 0)
            {
                correct_count++;
            }
            // } else {
            //     cout << row[6] << " " << row[8] <<endl;
            // }

            data.push_back(row);
        }

        file.close();

        cout << correct_count << "/" << data.size() << endl;
    }

    if (run_flag == "23")
    {
        std::ifstream file("./data/bssr1.csv");

        if (!file.is_open())
        {
            std::cout << "Failed to open the file." << std::endl;
            return -1;
        }

        std::vector<std::vector<std::string>> data, filteredData; // Store the CSV data as a 2D vector of strings
        std::unordered_set<string> set;

        std::string line;
        int lineno = 0;
        while (std::getline(file, line))
        {
            std::stringstream ss(line);
            std::vector<std::string> row;
            std::string cell;

            if (lineno == 0)
            {
                lineno++;
                continue;
            }

            int item = 0;
            string user_id_1;

            while (std::getline(ss, cell, ','))
            {
                if (item == 0)
                {
                    user_id_1 = cell;
                }

                row.push_back(cell);
                item++;
            }

            data.push_back(row);
            set.insert(user_id_1);
        }

        cout << data.size() << endl;

        file.close();

        vector<vector<vector<string>>> batches;

        vector<vector<string>> batch;

        for (int i = 0; i < data.size(); i++)
        {
            if (i > 0 && i % (int)(pow(2, logn)) == 0)
            {
                batches.push_back(batch);
                batch.clear();
            }
            batch.push_back(data[i]);
        }

        if (batch.size() > 0)
        {

            vector<vector<string>> newBatch;

            for (int t = 0; t < batch.size(); t++)
            {
                newBatch.push_back(batch[t]);
            }

            vector<string> dummyRow = {"0", "0", "0", "0", "0", "0", "0"};

            for (int rem = 0; rem < (pow(2, logn) - batch.size()); rem++)
            {
                newBatch.push_back(dummyRow);
            }

            batches.push_back(newBatch);
        }

        ofstream nist_file;
        nist_file.open("./data/nist_fusion_multiple_susim.csv", std::ios::out | ios::app);

        int subBatch = 0;

        int truePositive = 0;
        int trueNegative = 0;
        int falsePositive = 0;
        int falseNegative = 0;
        int doneIdx = 0;

        vector<complex<double>> scores1, scores2, scores3, scores4;
        Ciphertext cScores1, cScores2, cScores3, cScores4;

        Ciphertext ciphFusedScores;

        complex<double> *threshVals;

        Ciphertext ciphCompRes;
        Ciphertext ciphThresh;

        double avgCpuTime = 0;
        double avgElapsedTime = 0;

        for (int bchIdx = 0; bchIdx < batches.size(); bchIdx++)
        {

            scores1.clear();
            scores2.clear();
            scores3.clear();
            scores4.clear();

            for (const auto &row : batches[bchIdx])
            {
                scores1.push_back(complex<double>(stod(row[3]), 0.0));
                scores2.push_back(complex<double>(stod(row[4]), 0.0));
                scores3.push_back(complex<double>(stod(row[5]), 0.0));
                scores4.push_back(complex<double>(stod(row[6]), 0.0));
            }

            complex<double> *scores1Arr = new complex<double>[scores1.size()];
            complex<double> *scores2Arr = new complex<double>[scores2.size()];
            complex<double> *scores3Arr = new complex<double>[scores3.size()];
            complex<double> *scores4Arr = new complex<double>[scores4.size()];

            for (int i = 0; i < scores1.size(); i++)
            {
                scores1Arr[i] = scores1[i];
                scores2Arr[i] = scores2[i];
                scores3Arr[i] = scores3[i];
                scores4Arr[i] = scores4[i];
            }

            cScores1 = scheme.encrypt(scores1Arr, n, logp, logq);
            cScores2 = scheme.encrypt(scores2Arr, n, logp, logq);
            cScores3 = scheme.encrypt(scores3Arr, n, logp, logq);
            cScores4 = scheme.encrypt(scores4Arr, n, logp, logq);

            fheComp.scoreNormalize(
                cScores1, 0.0, 1.0,
                cScores1, scheme,
                n, logp, logq, secretKey);

            fheComp.scoreNormalize(
                cScores2, 0.0, 100.0,
                cScores2, scheme,
                n, logp, logq, secretKey);

            fheComp.scoreNormalize(
                cScores3, 0.0, 300.0,
                cScores3, scheme,
                n, logp, logq, secretKey);

            fheComp.scoreNormalize(
                cScores4, 0.0, 300.0,
                cScores4, scheme,
                n, logp, logq, secretKey);

            ciphFusedScores = scheme.add(cScores1, cScores2);
            scheme.addAndEqual(ciphFusedScores, cScores3);
            scheme.addAndEqual(ciphFusedScores, cScores4);

            threshVals = new complex<double>[scores1.size()];

            for (int i = 0; i < scores1.size(); i++)
            {
                threshVals[i] = complex<double>(0.386, 0.0);
            }

            double timeTkn;
            double multCount;

            scheme.multByConstAndEqual(ciphFusedScores, complex<double>(0.25, 0.0), logp);

            scheme.reScaleByAndEqual(ciphFusedScores, logp);
            scheme.reScaleByAndEqual(ciphFusedScores, logp);

            ciphThresh = scheme.encrypt(threshVals, n, ciphFusedScores.logp, ciphFusedScores.logq);

            // cout << ciphFusedScores.logp << " " << ciphFusedScores.logq <<endl;
            // cout << ciphPointFive.logp << " " << ciphPointFive.logq <<endl;

            auto start_time = std::chrono::high_resolution_clock::now();
            fheComp.newCompG(ciphFusedScores, ciphThresh,
                             ciphCompRes, scheme, n, ciphFusedScores.logp, ciphFusedScores.logq, timeTkn, 4, 2, 1, multCount, secretKey);

            auto end_time = std::chrono::high_resolution_clock::now();

            auto elapsed_time = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count();

            avgElapsedTime += elapsed_time;
            avgCpuTime += timeTkn;

            cout << "Time taken for " << batches[bchIdx].size() << " comparisons : " << timeTkn << endl;
            cout << "Elapsed time " << elapsed_time << " seconds" << endl;

            double unencAvg = 0.0;
            double unEncCompVal;

            complex<double> *decVals = scheme.decrypt(secretKey, ciphCompRes);
            complex<double> *decFusedScores = scheme.decrypt(secretKey, ciphFusedScores);

            stringstream buffer;

            for (int row_i = 0; row_i < batches[bchIdx].size(); row_i++)
            {
                unencAvg = (scores1[row_i].real() + scores2[row_i].real() / 100 + scores3[row_i].real() / 300 + scores4[row_i].real() / 300) / 4;
                if (unencAvg >= 0.386)
                {
                    unEncCompVal = 1;
                }
                else
                {
                    unEncCompVal = 0;
                }

                if (doneIdx < data.size())
                {
                    buffer << batches[bchIdx][row_i][0] << ", " << batches[bchIdx][row_i][1] << ", " << scores1[row_i].real() << ", " << scores2[row_i].real() << ", " << scores3[row_i].real() << ", " << scores4[row_i].real() << ", "
                           << unencAvg << ", " << unEncCompVal << ", "
                           << decFusedScores[row_i].real() << ", "
                           << decVals[row_i].real() << endl;
                }

                // double encComp = round(decVals[row_i].real());

                // if(doneIdx<data.size()){
                //     if(unEncCompVal == 1 && encComp == 1){
                //         truePositive++;
                //     }else if(unEncCompVal == 1 && encComp == 0){
                //         falseNegative++;
                //     }else if(unEncCompVal == 0 && encComp == 1){
                //         falsePositive++;
                //     }else{
                //         trueNegative++;
                //     }
                // }

                if (row_i > 0 && row_i % 10000 == 0)
                {
                    nist_file << buffer.rdbuf();
                    buffer.str("");
                    nist_file.flush();
                }
                doneIdx++;
            }

            if (!buffer.str().empty())
            {
                nist_file << buffer.rdbuf();
            }
        }
        cout << "Average CPU time " << avgCpuTime / batches.size();
        cout << "Average elapsed time " << avgElapsedTime / batches.size();
        nist_file.close();
    }

    if (run_flag == "24")
    {

        clock_t t1 = clock();
        cout << fib(44) << endl;
        clock_t t2 = clock();

        cout << ((double)(t2 - t1)) / CLOCKS_PER_SEC;
    }

    if (run_flag == "25")
    {
        complex<double> temp2;
        temp2.real(0.1);
        temp2.imag(0);
        // Encrypt Two Arry of Complex //
        Ciphertext cipher1;
        cipher1 = scheme.encryptSingle(temp2, logp, logq);

        Ciphertext ciphres;

            ciphres = algo.inverse(cipher1, logp, 8);

        cout << scheme.decryptSingle(secretKey, ciphres).real() << endl;
    }

    if (run_flag == "26")
    {
        complex<double> temp2;
        temp2.real(0.1);
        temp2.imag(0);

        Ciphertext cipher1;
        cipher1 = scheme.encryptSingle(temp2, logp, logq);

        Ciphertext ciphres;

        // fheComp.inverse(cipher1, scheme, ciphres, logp, 5, secretKey);
        // cout << ciphres.logp << ciphres.logq << endl;
        // cout << scheme.decryptSingle(secretKey, ciphres).real() << endl;
    }

    if (run_flag == "27")
    {

        vector<Ciphertext> ciphVals;

        vector<double> peakScores = {0.22889703, 0.24547607, 0.22302478, 0.18902795, 0.11453182,
                                     0.05944858, 0.03189783, 0.01099049, 0.00213614};

        vector<double> testScores = {0.6, 0.7, 1.2};

        double sumUnenc = 0.0;

        for (int i = 0; i < testScores.size(); i++)
        {
            complex<double> temp2;
            temp2.real(testScores[i]);
            temp2.imag(0);

            sumUnenc += testScores[i];

            Ciphertext cipher1;
            cipher1 = scheme.encryptSingle(temp2, logp, logq);
            ciphVals.push_back(cipher1);
        }

        cout << "Unencsum " << sumUnenc << endl;

        // vector<Ciphertext> res = fheComp.maxIdx(ciphVals, scheme, algo, logp, secretKey);
    }

    if (run_flag == "28")
    {
        // adding single constant to a cipher of vector
        vector<complex<double>> testvec;
        for (int i = 0; i < 10; i++)
        {
            complex<double> value = complex<double>(i, 0.0);
            testvec.push_back(value);
            cout << value << endl;
        }

        Ciphertext testCiph;
        complex<double>* testvec_array = testvec.data();
        testCiph = scheme.encrypt(testvec_array, n, logp, logq);

        complex<double> *beforeAdd = scheme.decrypt(secretKey, testCiph);

        cout << "Before Add" << endl;
        for (int i = 0; i < testvec.size(); i++)
        {
            cout << beforeAdd[i].real() << endl;
        }

        scheme.addConstAndEqual(testCiph, complex<double>(0.2, 0.0), logp);

        complex<double> *afterAdd = scheme.decrypt(secretKey, testCiph);

        cout << "After Add" << endl;
        for (int i = 0; i < testvec.size(); i++)
        {
            cout << afterAdd[i].real() << endl;
        }
    }

    if (run_flag == "29")
    {
        vector<double> scores1Vec = {0.3873817635704086, 0.7843225856878263, 0.7283883975116788, 0.8091855591394997, 0.7079666614093878, 0.6117975426642119, 0.840343658535564, 0.8836980628556513, 0.7819215679158983, 0.7818786489817396, 0.6674773576452229, 0.8912587739560927, 0.6728865390869364, 0.3970427109692421, 0.7794214527675382, 0.7865491362477266, 0.7178980329868646, 0.7257713893312601, 0.8825298304039125, 0.8095027405309665, 0.7542772381700762, 0.8964006716289794, 0.684974674339502, 0.6677073054306757, 0.8183311303940028, 0.8201288405467383, 0.816296144832909, 0.7774510200748953, 0.823653078229702, 0.8637578562585575, 0.6987603755650991, 0.7117204979433802, 0.778931548836164, 0.7419877677548304, 0.6663489037178266, 0.6795264122422942, 0.8151244230369296, 0.6841731719674458, 0.7317451466710957, 0.8954421487660978, 0.6622258945631829, 0.4230121554795962, 0.8074241381668656, 0.5946254328531528, 0.7728789323164915, 0.8223232891396252, 0.7851998068299029, 0.7863265160851788, 0.8051871195742438, 0.6536560651083717, 0.7425167523416988, 0.8319971471121489, 0.8535448946008679, 0.8116797424026505, 0.7888545459874633, 0.875725958069248, 0.7662216124399308, 0.7404384989120222, 0.6701690377923895, 0.8191263519464261, 0.7871974564076223, 0.8880485772547791, 0.9927090152093536, 0.5689735186686892, 0.6956314805845906, 0.8698683958926232, 0.721890191732487, 0.7363496853309365, 0.6697011167297308, 0.7759980571331268, 0.3926328777180248, 0.7596689728905864, 0.6892243466900775, 0.6695915513205772, 0.8844360591625291, 0.57260418135099, 0.7152893992326235, 0.6315064057381568, 0.7406011023536323, 0.5780768688578816, 0.7604512838693199, 0.7068905476455998, 0.7457527701903922, 0.698147995650881, 0.8020086759055196, 0.7169028720094575, 0.7291745467690767, 0.9338162120516365, 0.8020076291022477, 0.9235460251484015, 0.8103213406898014, 0.8012092671400078, 0.9582531365715379, 0.823460466427623, 0.7683508102955194, 0.6973761527048696, 0.766374096783243, 0.8856974571054911, 0.6696226064843182, 0.7685437710320225, 0.716614652175187, 0.6980869321266713, 0.7692754865192672, 0.7711747365894029, 0.3939923262341459, 0.7823078383233282, 0.7952512118492548, 0.8095107660227199, 0.7358472197602958, 0.8857944608753786, 0.7822453790614222, 0.7079286275571656, 0.7299610449608982, 0.6688385508334648, 0.8486999401228528, 0.8931555814852602, 0.837248959128613, 0.757132917496549, 0.8614182509452634, 0.8798357077157772, 0.7992639577258965, 0.7620647566461539, 0.7460333134673334, 0.7630428198367825, 0.7923861112933325, 0.8272478006663251, 0.705460963310243, 0.7701411928253499, 0.7471854949355654, 0.7364334295927096, 0.920547282708066, 0.7515314731871806, 0.7779070773671362, 0.8582949389155397, 0.907413739921029, 0.8307305151528264, 0.7952163184068491, 0.7423094852938098, 0.8383732258429206, 0.9278134931546045, 0.9051470619023624, 0.6896032894746024, 0.8807373542675376, 0.8653912182975625, 0.6468822011341766, 0.6787594543782195, 0.7431280854526447, 0.8400878896027312, 0.7812551031659518, 0.874343828815563, 0.652636827655705, 0.7200363031374786, 0.7453525424060006, 0.6403895783057699, 0.7524889492467902, 0.7872557284564398, 0.6314251040173517, 0.6969487080354014, 0.6732682733468531, 0.4151440331515618, 0.7443392368385422, 0.6377627999614776, 0.7437781502846603, 0.706278516665806, 0.34789704201310034, 0.769735382090173, 0.8496061228221258, 0.6635239306206706, 0.8385850290383228, 0.6771927388142089, 0.7332818538746373, 0.8235135044600795, 0.7375426921267831, 0.7716597554388408, 0.7798133061257532, 0.8825130815515575, 0.6779139862687322, 0.7510680882720348, 0.7692294271752922, 0.8243889809300358, 0.7898566856533519, 0.7880987540249584, 0.7817149987368571, 0.7902900622080289, 0.8509899967479311, 0.7893531732794391, 0.9024407265093853, 0.9583968975542488, 0.792326792441243, 0.782976047745395, 0.8144583072214073, 1.0, 0.8477850340629786, 0.8021922154125727, 0.7121988870387609, 0.7518224844968433, 0.7743978438644067, 0.8225651006954958, 0.8264916597693961, 0.8301708243366407, 0.6220792444034404, 0.7255651690866431, 0.9748020495012327, 0.9157222174922223, 0.9771154847327232, 0.8438082284320141, 0.6873718538327652, 0.79009570573383, 0.7443256283960041, 0.7564005041404559, 0.6086403839953549, 0.9054178350154299, 0.8293166328665522, 0.8575862531002824, 0.8051707196563135, 0.8268036071445019, 0.8668661641080465, 0.668272579197646, 0.7455772561750924, 0.7649438145790385, 0.7152611355442748, 0.7218619280441384, 0.8643112662551101, 0.7465015834644162, 0.6024220236242559, 0.8176531508080624, 0.7324695345354354, 0.7874186808324736, 0.767173854483179, 0.7557619541444334, 0.7821166222589454, 0.6777566168434832, 0.7659808476873322, 0.6893447290663768, 0.7285426265271113, 0.7017866838249353, 0.602402483296509, 0.7669226216978589, 0.8645408651061388, 0.8158697469667128, 0.8671721795979435, 0.696940682543648, 0.7373978843408001, 0.8414480359877006, 0.7110358886033826, 0.8857969034163469, 0.7367492152464803, 0.7268632051441307, 0.8352624754524633, 0.7879204485342659, 0.8323335198969384, 0.6786359315921038, 0.9050832069027602, 0.6666981870763063, 0.7893908581972372, 0.8558482107340601, 0.9014106720895728, 0.7963266277441946, 0.8331657284983118, 0.768317661525234, 0.8596491534153001, 0.6774596736486116, 0.6974455906552569, 0.7192826047815183, 0.8417865023790346, 0.8664701235367435, 0.880607899596213, 0.7736825282950923, 0.7682098407882008, 0.8762689000330786, 0.791728718838411, 0.7682461299683027, 0.8319259644896413, 0.7995975390352937, 0.822735380694435, 0.8538673100086952, 0.7992590726439598, 0.6564838296809201, 0.8099783381509543, 0.8509819712561777, 0.8960182395002143, 0.8138047530451505, 0.38881413938115766, 0.9126809050521514, 0.8064596834187757, 0.6878415195675447, 0.657544939264474, 0.5805958264651407, 0.7630668963120419, 0.9065396591887694, 0.7777392399091652, 0.7768428273737659, 0.6000911416715631, 0.8312814826084104, 0.6435271966468796, 0.6805700751046452, 0.9530728561120052, 0.8531840964063943, 0.6772391470926082, 0.7570683646280986, 0.6448388411469057, 0.7694314602068202, 0.7864674855924975, 0.8220326267643867, 0.9389113525116998, 0.6463144848262375, 0.7550030217721121, 0.786578097804923, 0.6886625622673478, 0.7527520458025281, 0.7997378106737646, 0.7628229911496269, 0.7430635325841943, 0.7437844311042934, 0.7960987735652862, 0.7852870404359168, 0.7363926042650951, 0.6698532521386188, 0.9283997029870179, 0.8176967676110694, 0.782089754308293, 0.883203622776764, 0.8443857149038267, 0.680066562730733, 0.8899659719149661, 0.710611584343731, 0.85840938940663, 0.6315807287704804, 0.8265649359984478, 0.6981595104868751, 0.7283723465281722, 0.7172577383187223, 0.8483593801249742, 0.7800320880096362, 0.5996249652810248, 0.81828960719754, 0.6997548386736584, 0.7784887510520374, 0.7623557679558166, 0.802135339101452, 0.7759157086090498, 0.7417208329204272, 0.8064851556317315, 0.8356972477448366, 0.6966576967257387, 0.7676361925950529, 0.7048196218388286, 0.7375378070448463, 0.6549813180509358, 0.8103782170009223, 0.7736884601803011, 0.8689573281114134, 0.7238930753265674, 0.6185218579501918, 0.6691361918971841, 0.7225985286133205, 0.6197563879425009, 0.8078599572625114, 0.6263310103605609, 0.7873576173082637, 0.70450488298833, 0.7567469960235432, 0.8731728048884314, 0.6835398559877842, 0.8657541100985807, 0.7301037591403372, 0.7847371197836048, 0.7908207914670181, 0.8200133432523758, 0.6815680275574447, 0.6963907618913361, 0.7741504493577513, 0.8816508645897156, 0.8362886915936115, 0.7017528371858021, 0.7187072119162501, 0.6623815193163117, 0.7411538144813365, 0.8041029803187024, 0.682718464353557, 0.8348831837335143, 0.8925658823086057, 0.676923012504414, 0.5815110814594391, 0.8596714852184398, 0.7135464717844643, 0.7397856426046138, 0.798178422732659, 0.7143849612054708, 0.7734546741161838, 0.8774252687143997, 0.5915607418066707, 0.7012018697302176, 0.7806671486614176, 0.7247518029441692, 0.6981514849951218, 0.8369310798682978, 0.7633055674580965, 0.7199989671541047, 0.7725648913348417, 0.9999829022132213, 0.8597018425133325, 0.8536555068132935, 0.7653579997403928, 0.809023653566738, 0.8487801950403857, 0.8430503428629648, 0.9273961675834336, 0.8383156516629515, 0.678541719297609, 0.8274369231241634, 0.6367180902958545, 0.8393823441972901, 0.8442744048225529, 0.7599104355120333, 0.8512000552712129, 0.80514978359087, 0.9174986426450901, 0.7953356539798759, 0.7411042657931208, 0.7564493549598234, 0.6472771949022077, 0.630977770085712, 0.8385445526451324, 0.9110876704719126, 0.8134156911623283, 0.8234001007722616, 0.8050915115420529, 0.7639465599950868, 0.6006176837174633, 0.8547916372980189, 0.8910225453510068, 0.7928774109624028, 0.8012671902544012, 0.664340437172961, 0.8056400364566683, 0.8017556984480791, 0.8562882170427948, 0.7948928561957493, 0.6325263410596719, 0.8579805489994652, 0.5560374727656681, 0.9440895393646879, 0.7198960314990083, 0.6207473617068197, 0.7522590014613374, 0.728264874725563, 0.9302281193690706, 0.870369116791143, 0.7842112756065523, 0.7050516632308256, 0.8173506246624057, 0.8677001173815404, 0.7971344109358839, 0.7983172986334331, 0.4057402504232572, 0.8312378658054035, 0.706805756580554, 0.7316491897044801, 0.8026650215571687, 0.7700159253671137, 0.7841296249513237, 0.6599400251511932, 0.6910517162688582, 0.7991809113329712, 0.8385141953502393, 0.952922465375237, 0.7862860396919883, 0.7636429870461585, 0.851420232892792, 0.8432098058947584, 0.6066050494998374, 0.8347844352915069, 0.9302668510901406, 0.7789967995734622, 0.7002660276048998, 0.7971448789686054, 0.7016212889079328, 0.6265257157691839, 0.7540619456304338, 0.6852744090097659, 0.7151826252988622, 0.4030175151123497, 0.8839629040835094, 0.7048534684779619, 0.7392280453949727, 0.806405947517471, 0.7556830949645971, 0.6581370809820968, 0.8141167004202563, 0.9566281489587098, 0.9254149179236446, 0.38775268086318015, 0.8931684920589504, 0.8779901935469464, 0.7993414211680367, 0.7971392960178206, 0.7231030877905053, 0.7565240269265717, 0.8523250198543684, 0.6827564982057792, 0.7380465534351195, 0.7428597548805455, 0.4245729391583979, 0.7377750824532042, 0.8547427864786514, 0.6512791238117037, 0.7087713041912606, 0.9519880189876154, 0.8199589094822233, 0.6741479370298978, 0.7592701408438909, 0.776781763849556};

        vector<double> gen_quantiles = {0.6242051273820006,
                                        0.668272579197646,
                                        0.6889434544787126,
                                        0.7083689828003241,
                                        0.7443256283960041,
                                        0.7759157086090498,
                                        0.7992639577258965,
                                        0.8312596742069069,
                                        0.8689573281114134,
                                        1.0};

        vector<double> imp_quantiles = {0.30641187940268005,
                                        0.3386977348572929,
                                        0.35415744898229773,
                                        0.36502483715230416,
                                        0.38171157917950177,
                                        0.39557369704396694,
                                        0.4093918491710016,
                                        0.4286373273298002,
                                        0.5919470122141006,
                                        0.7553236925078195};

        complex<double> *scores1Arr = new complex<double>[scores1Vec.size()];

        for (int i = 0; i < scores1Vec.size(); i++)
        {
            scores1Arr[i] = complex<double>(scores1Vec[i], 0.0);
        }

        Ciphertext ciphScores1;
        ciphScores1 = scheme.encrypt(scores1Arr, scores1Vec.size(), logp, logq);

        vector<Ciphertext> quantileComps;

        for (int i = 0; i < gen_quantiles.size(); i++)
        {
            Ciphertext ciphRes;
            fheComp.compGAgainstConst(
                ciphScores1, gen_quantiles[i],
                ciphRes, scheme, 512, logp,
                logq, 3, 2, 2, secretKey);
            quantileComps.push_back(ciphRes);
        }

        Ciphertext ciphRes;
        fheComp.compGAgainstConst(
            ciphScores1, 0.2,
            ciphRes, scheme, 4, logp,
            logq, 3, 2, 2, secretKey);

        complex<double> *afterComp = scheme.decrypt(secretKey, ciphRes);
    }

    if (run_flag == "30")
    {
        vector<double> means = {0.00068709, 0.00074056, 0.00080664, 0.0006481};
        vector<double> variances = {8.69343728e-05, 9.35118951e-05, 9.24932353e-05, 8.45180489e-05};

        std::ifstream file("./data/preprocessed_dataset.csv");

        if (!file.is_open())
        {
            std::cout << "Failed to open the file." << std::endl;
            return -1;
        }

        std::vector<std::vector<std::string>> data;

        std::string line;
        int lineno = 0;
        while (std::getline(file, line))
        {
            std::stringstream ss(line);
            std::vector<std::string> row;
            std::string cell;

            if (lineno == 0)
            {
                lineno++;
                continue;
            }

            while (std::getline(ss, cell, ','))
            {
                row.push_back(cell);
            }
            data.push_back(row);
        }

        file.close();

        vector<double> mod1GenQuantiles;
        vector<double> mod2GenQuantiles;
        vector<double> mod3GenQuantiles;
        vector<double> mod4GenQuantiles;

        vector<double> mod1ImpQuantiles;
        vector<double> mod2ImpQuantiles;
        vector<double> mod3ImpQuantiles;
        vector<double> mod4ImpQuantiles;

        vector<vector<string>> quantiles;

        std::ifstream file2("./data/quantiles.csv");

        if (!file2.is_open())
        {
            std::cout << "Failed to open the file." << std::endl;
            return -1;
        }

        lineno = 0;
        while (std::getline(file2, line))
        {
            std::stringstream ss(line);
            std::vector<std::string> row;
            std::string cell;

            if (lineno == 0)
            {
                lineno++;
                continue;
            }

            while (std::getline(ss, cell, ','))
            {
                row.push_back(cell);
            }
            quantiles.push_back(row);
        }

        file2.close();

        for (int i = 0; i < quantiles.size(); i++)
        {
            mod1GenQuantiles.push_back(stod(quantiles[i][0]));
            mod2GenQuantiles.push_back(stod(quantiles[i][1]));
            mod3GenQuantiles.push_back(stod(quantiles[i][2]));
            mod4GenQuantiles.push_back(stod(quantiles[i][3]));

            mod1ImpQuantiles.push_back(stod(quantiles[i][4]));
            mod2ImpQuantiles.push_back(stod(quantiles[i][5]));
            mod3ImpQuantiles.push_back(stod(quantiles[i][6]));
            mod4ImpQuantiles.push_back(stod(quantiles[i][7]));
        }

        vector<vector<vector<string>>> batches;

        vector<vector<string>> batch;

        for (int i = 0; i < data.size(); i++)
        {

            if (i != 0 && i % int(pow(2, 14)) == 0)
            {
                batches.push_back(batch);
                batch.clear();
            }
            batch.push_back(data[i]);
        }

        for (int i = 0; i < batches.size(); i++)
        {
            complex<double> *mod1_scores = new complex<double>[batches[i].size()];
            complex<double> *mod2_scores = new complex<double>[batches[i].size()];
            complex<double> *mod3_scores = new complex<double>[batches[i].size()];
            complex<double> *mod4_scores = new complex<double>[batches[i].size()];
            vector<string> y_label;

            vector<vector<string>> batch = batches[i];
            for (int j = 0; j < batch.size(); j++)
            {
                mod1_scores[j] = complex<double>(stod(batch[j][0]), 0.0);
                mod2_scores[j] = complex<double>(stod(batch[j][1]), 0.0);
                mod3_scores[j] = complex<double>(stod(batch[j][2]), 0.0);
                mod4_scores[j] = complex<double>(stod(batch[j][3]), 0.0);
                y_label.push_back(batch[j][4]);
            }

            Ciphertext ciphMod1Scores;
            Ciphertext ciphMod2Scores;
            Ciphertext ciphMod3Scores;
            Ciphertext ciphMod4Scores;

            ciphMod1Scores = scheme.encrypt(mod1_scores, batch.size(), logp, logq);
            ciphMod2Scores = scheme.encrypt(mod2_scores, batch.size(), logp, logq);
            ciphMod3Scores = scheme.encrypt(mod3_scores, batch.size(), logp, logq);
            ciphMod4Scores = scheme.encrypt(mod4_scores, batch.size(), logp, logq);

            // fheComp.standardScalar(ciphMod1Scores, means[0], variances[0], scheme, batch.size(), logp, logq);
            // fheComp.standardScalar(ciphMod2Scores, means[1], variances[1], scheme, batch.size(), logp, logq);
            // fheComp.standardScalar(ciphMod3Scores, means[2], variances[2], scheme, batch.size(), logp, logq);
            // fheComp.standardScalar(ciphMod4Scores, means[3], variances[3], scheme, batch.size(), logp, logq);

            int batchSize = batches[i].size();

            cout << batchSize <<endl;

            fheMathCore.getQPLC(ciphMod1Scores,
                                mod1GenQuantiles,
                                mod1ImpQuantiles, scheme, algo,
                                batchSize, logp, logq, secretKey);
        }
    }

    if (run_flag == "31")
    {
        complex<double> *test = new complex<double>[4];
        test[0] = complex<double>(0.1, 1.0);
        test[1] = complex<double>(0.3, 1.0);
        test[2] = complex<double>(0.5, 1.0);
        test[3] = complex<double>(0.9, 1.0);
        // vector<double> quantiles = {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0};
        vector<double> quantiles = {0.3, 0.6, 0.9, 1.0};
        Ciphertext testciph;
        cout << "Encrypting test data" << endl;
        testciph = scheme.encrypt(test, 4, logp, logq);

        auto startTime = std::chrono::high_resolution_clock::now();

        fheMathCore.getQPLC(testciph, quantiles, quantiles, scheme, algo, 4, logp, logq, secretKey);

        auto endTime = std::chrono::high_resolution_clock::now();

        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        std::cout << "Time taken by qplc function: " << duration.count() << " milliseconds" << std::endl;
    }

    if (run_flag == "32")
    {
        complex<double> *test1 = new complex<double>[4];
        test1[0] = complex<double>(0.81, 0.0);
        test1[1] = complex<double>(0.7, 0.0);
        test1[2] = complex<double>(0.96, 0.0);
        test1[3] = complex<double>(1.2, 0.0);

        complex<double> *test2 = new complex<double>[4];
        test2[0] = complex<double>(0.54, 0.0);
        test2[1] = complex<double>(0.62, 0.0);
        test2[2] = complex<double>(0.78, 0.0);
        test2[3] = complex<double>(1.4, 0.0);

        complex<double> *test3 = new complex<double>[4];
        test3[0] = complex<double>(0.59, 0.0);
        test3[1] = complex<double>(0.55, 0.0);
        test3[2] = complex<double>(0.89, 0.0);
        test3[3] = complex<double>(1.1, 0.0);

        complex<double> *test4 = new complex<double>[4];
        test4[0] = complex<double>(0.51, 0.0);
        test4[1] = complex<double>(0.78, 0.0);
        test4[2] = complex<double>(0.56, 0.0);
        test4[3] = complex<double>(1.332, 0.0);

        Ciphertext testCiph1, testCiph2, testCiph3, testCiph4;
        Ciphertext maxIdx;
        testCiph1 = scheme.encrypt(test1, 4, logp, logq);
        testCiph2 = scheme.encrypt(test2, 4, logp, logq);
        testCiph3 = scheme.encrypt(test3, 4, logp, logq);
        testCiph4 = scheme.encrypt(test4, 4, logp, logq);

        vector<Ciphertext> ciphVec = {testCiph1, testCiph2, testCiph3, testCiph4};

        cout << "Start " << ciphVec[0].logp << " " << ciphVec[0].logq << endl;

        auto startTime = std::chrono::high_resolution_clock::now();

        fheMathCore.getMaxIdx(ciphVec, maxIdx, scheme, algo, 4, logp, logq, secretKey);

        auto endTime = std::chrono::high_resolution_clock::now();

        // Calculate the duration
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

        // Print the elapsed time
        std::cout << "Time taken by getMaxIdx function: " << duration.count() << " milliseconds" << std::endl;

        cout << "End " << maxIdx.logp << " " << maxIdx.logq << endl;

        complex<double> *maxDec = scheme.decrypt(secretKey, maxIdx);
        for (int i = 0; i < 4; i++)
        {
            cout << maxDec[i].real() << " ";
        }
    }

    if (run_flag == "33")
    {
        vector<int> testvec;
        for (int i = 0; i < 5; i++)
        {
            testvec.push_back(i);
        }
        cout << "Size of : " << testvec.size() << endl;
    }

    if (run_flag == "34")
    {
        complex<double> *cmplx = new complex<double>[1];
        cmplx[0] = complex<double>(0.5, 0.0);
        Ciphertext ciph;
        Ciphertext ciphCpy;
        Ciphertext invRes;
        ciph = scheme.encrypt(cmplx, 1, logp, logq);

        fheMathCore.inverse(ciph, scheme, invRes, logp, 5, secretKey);
        cout << scheme.decrypt(secretKey, invRes)[0].real() << endl;
    }

    if (run_flag == "36")
    {
        cout << "Starting SQPT (Score Quantile Power Transform) for balanced_sample_1000_4000.csv" << endl;
        
        std::ifstream file("../embedding_databases/balanced_sample_1000_4000.csv");

        if (!file.is_open())
        {
            std::cout << "Failed to open the balanced_sample_1000_4000.csv file." << std::endl;
            return -1;
        }

        std::vector<std::vector<std::string>> data;
        std::unordered_set<string> set;

        std::string line;
        int lineno = 0;
        while (std::getline(file, line))
        {
            std::stringstream ss(line);
            std::vector<std::string> row;
            std::string cell;

            if (lineno == 0)
            {
                lineno++;
                continue; // Skip header
            }

            while (std::getline(ss, cell, ','))
            {
                row.push_back(cell);
            }
            data.push_back(row);
        }

        cout << "Loaded " << data.size() << " records for SQPT" << endl;
        file.close();

        // Load actual quantiles computed from training data
        vector<double> biography_genuine_quantiles;
        vector<double> biometric_genuine_quantiles;
        vector<double> biography_impostor_quantiles;
        vector<double> biometric_impostor_quantiles;

        // Read quantiles from the computed file
        std::ifstream quantile_file("/home/csgrad/susimmuk/authentication/data/2score_quantiles.csv");
        if (!quantile_file.is_open())
        {
            std::cout << "Failed to open 2score_quantiles.csv file. Using default quantiles." << std::endl;
            // Fallback to default quantiles
            biography_genuine_quantiles = {0.1, 0.3, 0.5, 0.7, 0.9};
            biography_impostor_quantiles = {0.15, 0.35, 0.55, 0.75, 0.95};
            biometric_genuine_quantiles = {0.2, 0.4, 0.6, 0.8, 1.0};
            biometric_impostor_quantiles = {0.25, 0.45, 0.65, 0.85, 1.05};
        }
        else
        {
            std::string header_line;
            std::getline(quantile_file, header_line); // Skip header
            
            std::string quant_line;
            while (std::getline(quantile_file, quant_line))
            {
                std::stringstream ss(quant_line);
                std::string cell;
                vector<string> row;
                
                while (std::getline(ss, cell, ','))
                {
                    row.push_back(cell);
                }
                
                if (row.size() >= 4)
                {
                    biography_genuine_quantiles.push_back(stod(row[0]));
                    biometric_genuine_quantiles.push_back(stod(row[1]));
                    biography_impostor_quantiles.push_back(stod(row[2]));
                    biometric_impostor_quantiles.push_back(stod(row[3]));
                }
            }
            quantile_file.close();
            
            cout << "Loaded " << biography_genuine_quantiles.size() << " quantiles from computed file" << endl;
            cout << "Biography genuine quantiles range: " << biography_genuine_quantiles[0] 
                 << " to " << biography_genuine_quantiles.back() << endl;
            cout << "Biometric genuine quantiles range: " << biometric_genuine_quantiles[0] 
                 << " to " << biometric_genuine_quantiles.back() << endl;
        }

        // Create batches
        vector<vector<vector<string>>> batches;
        vector<vector<string>> batch;

        for (int i = 0; i < data.size(); i++)
        {
            if (i > 0 && i % (int)(pow(2, logn)) == 0)
            {
                batches.push_back(batch);
                batch.clear();
            }
            batch.push_back(data[i]);
        }

        if (batch.size() > 0)
        {
            vector<vector<string>> newBatch;
            for (int t = 0; t < batch.size(); t++)
            {
                newBatch.push_back(batch[t]);
            }

            // Pad with dummy rows
            vector<string> dummyRow = {"0", "0", "0", "0", "0"};
            for (int rem = 0; rem < (pow(2, logn) - batch.size()); rem++)
            {
                newBatch.push_back(dummyRow);
            }
            batches.push_back(newBatch);
        }

        // Output file for SQPT results
        ofstream sqpt_file;
        sqpt_file.open("/home/csgrad/susimmuk/authentication/data/sqpt_results.csv", std::ios::out);
        sqpt_file << "query_id,gallery_id,biography_score,biometric_score,true_label,bio_bin,biom_bin,sqpt_score,decision" << endl;

        double avgCpuTime = 0;
        double avgElapsedTime = 0;

        for (int bchIdx = 0; bchIdx < batches.size(); bchIdx++)
        {
            cout << "Processing SQPT batch " << (bchIdx + 1) << "/" << batches.size() << endl;
            
            vector<complex<double>> biography_scores, biometric_scores;
            
            // Extract scores from batch
            for (const auto &row : batches[bchIdx])
            {
                biography_scores.push_back(complex<double>(stod(row[2]), 0.0)); // biography_score
                biometric_scores.push_back(complex<double>(stod(row[3]), 0.0)); // biometric_score
            }

            // Convert to arrays for encryption
            complex<double> *bio_scores_arr = new complex<double>[biography_scores.size()];
            complex<double> *biom_scores_arr = new complex<double>[biometric_scores.size()];

            for (int i = 0; i < biography_scores.size(); i++)
            {
                bio_scores_arr[i] = biography_scores[i];
                biom_scores_arr[i] = biometric_scores[i];
            }

            // Encrypt the scores
            Ciphertext ciphBioScores, ciphBiomScores;
            ciphBioScores = scheme.encrypt(bio_scores_arr, n, logp, logq);
            ciphBiomScores = scheme.encrypt(biom_scores_arr, n, logp, logq);

            auto start_time = std::chrono::high_resolution_clock::now();

            cout << "Applying quantile binning to biography scores..." << endl;
            // Apply quantile binning to biography scores (Algorithm 1)
            fheMathCore.getQPLC(ciphBioScores,
                                biography_genuine_quantiles,
                                biography_impostor_quantiles, 
                                scheme, algo,
                                biography_scores.size(), logp, logq, secretKey);

            cout << "Applying quantile binning to biometric scores..." << endl;
            // Apply quantile binning to biometric scores (Algorithm 1)  
            fheMathCore.getQPLC(ciphBiomScores,
                                biometric_genuine_quantiles,
                                biometric_impostor_quantiles,
                                scheme, algo, 
                                biometric_scores.size(), logp, logq, secretKey);

            auto end_time = std::chrono::high_resolution_clock::now();
            auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
            avgElapsedTime += elapsed_time;

            cout << "Time taken for SQPT on " << batches[bchIdx].size() << " samples: " << elapsed_time << " ms" << endl;

            // For now, we'll implement a simplified version since the full SQPT requires 
            // additional steps (normalization and power transform) that aren't fully implemented
            // The quantile binning outputs are saved to files by getQPLC function

            // Process results (simplified - in full implementation, you'd combine the quantile bins)
            stringstream buffer;
            for (int row_i = 0; row_i < batches[bchIdx].size(); row_i++)
            {
                if (row_i < data.size()) // Only process real data, not dummy rows
                {
                    // For demo purposes, using original scores as SQPT transformed scores
                    double sqpt_score = (biography_scores[row_i].real() + biometric_scores[row_i].real()) / 2.0;
                    double decision = (sqpt_score >= 0.5) ? 1.0 : 0.0;

                    buffer << batches[bchIdx][row_i][0] << "," << batches[bchIdx][row_i][1] << ","
                           << biography_scores[row_i].real() << "," << biometric_scores[row_i].real() << ","
                           << batches[bchIdx][row_i][4] << "," << "bin_pending" << "," << "bin_pending" << ","
                           << sqpt_score << "," << decision << endl;
                }
            }

            sqpt_file << buffer.rdbuf();

            // Clean up
            delete[] bio_scores_arr;
            delete[] biom_scores_arr;
        }

        cout << "Average elapsed time: " << avgElapsedTime / batches.size() << " ms" << endl;
        
        sqpt_file.close();
        cout << "SQPT processing completed!" << endl;
        cout << "Quantile binning results saved to:" << endl;
        cout << "  - /home/csgrad/susimmuk/authentication/data/computed_comps.csv (comparison results)" << endl;
        cout << "  - /home/csgrad/susimmuk/authentication/data/one_minus_computed_comps.csv (1-comparison results)" << endl;
        cout << "  - /home/csgrad/susimmuk/authentication/data/prods.csv (product results)" << endl;
        cout << "  - /home/csgrad/susimmuk/authentication/data/max_results.csv (max index results - final bins)" << endl;
        cout << "  - /home/csgrad/susimmuk/authentication/data/sqpt_results.csv (summary results)" << endl;
    }

    if (run_flag == "37")
    {
        cout << "Starting 2-Score Fusion (like run_flag 23) on similarity.csv" << endl;

        std::ifstream file("/home/csgrad/susimmuk/authentication/embedding_databases/similarity.csv");

        if (!file.is_open())
        {
            std::cout << "Failed to open the similarity.csv file." << std::endl;
            return -1;
        }

        std::vector<std::vector<std::string>> data;
        std::unordered_set<string> set;

        std::string line;
        int lineno = 0;
        while (std::getline(file, line))
        {
            std::stringstream ss(line);
            std::vector<std::string> row;
            std::string cell;

            if (lineno == 0)
            {
                lineno++;
                continue; // Skip header
            }

            int item = 0;
            string query_id;

            while (std::getline(ss, cell, ','))
            {
                if (item == 0)
                {
                    query_id = cell;
                }
                row.push_back(cell);
                item++;
            }

            data.push_back(row);
            set.insert(query_id);
        }

        cout << "Loaded " << data.size() << " records" << endl;
        file.close();

        // Create batches (same as run_flag 23)
        vector<vector<vector<string>>> batches;
        vector<vector<string>> batch;

        for (int i = 0; i < data.size(); i++)
        {
            if (i > 0 && i % (int)(n) == 0)
            {
                batches.push_back(batch);
                batch.clear();
            }
            batch.push_back(data[i]);
        }

        if (batch.size() > 0)
        {
            vector<vector<string>> newBatch;
            for (int t = 0; t < batch.size(); t++)
            {
                newBatch.push_back(batch[t]);
            }

            // Pad with dummy rows (adjust for 2 scores instead of 4)
            vector<string> dummyRow = {"0", "0", "0", "0", "0"};
            for (int rem = 0; rem < (n - batch.size()); rem++)
            {
                newBatch.push_back(dummyRow);
            }
            batches.push_back(newBatch);
        }

        // Output file
        ofstream fusion_file;
        fusion_file.open("/home/csgrad/susimmuk/authentication/data/separate_4_2_1.csv", std::ios::out);
        fusion_file << "query_id,gallery_id,biography_score,biometric_score,true_label,fused_score,unenc_decision,enc_fused_score,enc_decision" << endl;

        int doneIdx = 0;
        double avgCpuTime = 0;
        double avgElapsedTime = 0;
        // #pragma omp parallel for num_threads(numThread) schedule(dynamic)
        // auto start_time = std::chrono::high_resolution_clock::now();
        for (int bchIdx = 0; bchIdx < batches.size(); bchIdx++)
        {
            cout << "Processing batch " << (bchIdx + 1) << "/" << batches.size() << endl;

            vector<complex<double>> scores1, scores2; // biography and biometric scores
            // #pragma omp critical
            {
                // Extract scores from batch
                for (const auto &row : batches[bchIdx])
                {
                    scores1.push_back(complex<double>(stod(row[2]), 0.0)); // biography_score
                    scores2.push_back(complex<double>(stod(row[3]), 0.0)); // biometric_score
                }
            }
            // for (int i = 0; i < batches[bchIdx].size(); i++)
            // {
            //     scores1.push_back(complex<double>(stod(batches[bchIdx][i][2]), 0.0)); // biography_score
            //     scores2.push_back(complex<double>(stod(batches[bchIdx][i][3]), 0.0)); // biometric_score
            // }
            // Convert to arrays for encryption
            complex<double> *scores1Arr = new complex<double>[scores1.size()];
            complex<double> *scores2Arr = new complex<double>[scores2.size()];

            for (int i = 0; i < scores1.size(); i++)
            {
                scores1Arr[i] = scores1[i];
                scores2Arr[i] = scores2[i];
            }

            // Encrypt the scores
            Ciphertext cScores1, cScores2;
            cScores1 = scheme.encrypt(scores1Arr, n, logp, logq);
            cScores2 = scheme.encrypt(scores2Arr, n, logp, logq);

            // can u decrypt here and check if encryption preserves the values
            Ciphertext ciphFusedScores = scheme.add(cScores1, cScores2);

            scheme.multByConstAndEqual(ciphFusedScores, complex<double>(0.5, 0.0), logp);
            scheme.reScaleByAndEqual(ciphFusedScores, logp);
            // scheme.reScaleByAndEqual(ciphFusedScores, logp);

            // Set threshold for comparison (0.7621382391966248)
            complex<double> *threshVals = new complex<double>[scores1.size()];
            for (int i = 0; i < scores1.size(); i++)
            {
                threshVals[i] = complex<double>(0.7496399657833722, 0.0);
            }

            Ciphertext ciphThresh = scheme.encrypt(threshVals, n, ciphFusedScores.logp, ciphFusedScores.logq);

            // Perform encrypted comparison
            Ciphertext ciphCompRes;
            double timeTkn, multCount;

            // check if the ciphFusedScores is same as the one you got after addition and division
            // complex<double> *decFusedScores3 = scheme.decrypt(secretKey, ciphFusedScores);


            auto start_time = std::chrono::high_resolution_clock::now();
            fheComp.newCompG(ciphFusedScores, ciphThresh, ciphCompRes, scheme, n, 
                           ciphFusedScores.logp, ciphFusedScores.logq, timeTkn, 4, 2, 1, multCount, secretKey);
            auto end_time = std::chrono::high_resolution_clock::now();
            auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
            avgElapsedTime += elapsed_time;
            avgCpuTime += timeTkn;

            cout << "Time taken for " << batches[bchIdx].size() << " comparisons: " << timeTkn << " ms" << endl;
            cout << "Elapsed time: " << elapsed_time << " ms" << endl;

            // Decrypt results for verification
            complex<double> *decVals = scheme.decrypt(secretKey, ciphCompRes);
            complex<double> *decFusedScores = scheme.decrypt(secretKey, ciphFusedScores);

            // Process results
            stringstream buffer;
            for (int row_i = 0; row_i < batches[bchIdx].size(); row_i++)
            {
                if (doneIdx < data.size()) // Only process real data, not dummy rows
                {
                    // Unencrypted fusion: add scores and divide by 2
                    double unencFusedScore = (scores1[row_i].real() + scores2[row_i].real()) / 2.0;
                    double unEncDecision = (unencFusedScore >= 0.7496399657833722) ? 1.0 : 0.0;

                    buffer << batches[bchIdx][row_i][0] << "," << batches[bchIdx][row_i][1] << ","
                           << scores1[row_i].real() << "," << scores2[row_i].real() << ","
                           << batches[bchIdx][row_i][4] << "," << unencFusedScore << ","
                           << unEncDecision << "," << decFusedScores[row_i].real() << ","
                           << decVals[row_i].real() << endl;
                }

                if (row_i > 0 && row_i % 1000 == 0)
                {
                    fusion_file << buffer.rdbuf();
                    buffer.str("");
                    fusion_file.flush();
                }
                doneIdx++;
            }

            if (!buffer.str().empty())
            {
                fusion_file << buffer.rdbuf();
            }

            // Clean up
            delete[] scores1Arr;
            delete[] scores2Arr;
            delete[] threshVals;

        }
        cout << "Average CPU time: " << avgCpuTime / batches.size() << " ms" << endl;
        cout << "Average elapsed time: " << avgElapsedTime / batches.size() << " ms" << endl;
        
        fusion_file.close();
        cout << "2-Score fusion (like run_flag 23) completed! Results saved to /home/csgrad/susimmuk/authentication/data/separate_4_2_1.csv" << endl;
    }

    if (run_flag == "38")
    {
        cout << "Starting 1-Score Fusion (like run_flag 23) on fused_similarity.csv" << endl;

        std::ifstream file("/home/csgrad/susimmuk/authentication/embedding_databases/fused_similarity.csv");

        if (!file.is_open())
        {
            std::cout << "Failed to open the similarity.csv file." << std::endl;
            return -1;
        }

        std::vector<std::vector<std::string>> data;
        std::unordered_set<string> set;

        std::string line;
        int lineno = 0;
        while (std::getline(file, line))
        {
            std::stringstream ss(line);
            std::vector<std::string> row;
            std::string cell;

            if (lineno == 0)
            {
                lineno++;
                continue; // Skip header
            }

            int item = 0;
            string query_id;

            while (std::getline(ss, cell, ','))
            {
                if (item == 0)
                {
                    query_id = cell;
                }
                row.push_back(cell);
                item++;
            }

            data.push_back(row);
            set.insert(query_id);
        }

        cout << "Loaded " << data.size() << " records" << endl;
        file.close();

        // Create batches (same as run_flag 23)
        vector<vector<vector<string>>> batches;
        vector<vector<string>> batch;

        for (int i = 0; i < data.size(); i++)
        {
            if (i > 0 && i % (int)(n) == 0)
            {
                batches.push_back(batch);
                batch.clear();
            }
            batch.push_back(data[i]);
        }

        if (batch.size() > 0)
        {
            vector<vector<string>> newBatch;
            for (int t = 0; t < batch.size(); t++)
            {
                newBatch.push_back(batch[t]);
            }

            // Pad with dummy rows (adjust for 2 scores instead of 4)
            vector<string> dummyRow = {"0", "0", "0", "0"};
            for (int rem = 0; rem < (n - batch.size()); rem++)
            {
                newBatch.push_back(dummyRow);
            }
            batches.push_back(newBatch);
        }

        // Output file
        ofstream fusion_file;
        fusion_file.open("/home/csgrad/susimmuk/authentication/data/fusion_feature_1_1_1.csv", std::ios::out);
        fusion_file << "query_id,gallery_id,feature_level_score,true_label,fused_score,unenc_decision,enc_fused_score,enc_decision" << endl;

        int doneIdx = 0;
        double avgCpuTime = 0;
        double avgElapsedTime = 0;

        for (int bchIdx = 0; bchIdx < batches.size(); bchIdx++)
        {
            cout << "Processing batch " << (bchIdx + 1) << "/" << batches.size() << endl;

            vector<complex<double>> scores1; // biography and biometric scores
            
            // Extract scores from batch
            for (const auto &row : batches[bchIdx])
            {
                scores1.push_back(complex<double>(stod(row[2]), 0.0)); // fused score
            }

            // Convert to arrays for encryption
            complex<double> *scores1Arr = new complex<double>[scores1.size()];

            for (int i = 0; i < scores1.size(); i++)
            {
                scores1Arr[i] = scores1[i];
            }

            // Encrypt the scores
            Ciphertext cScores1;
            cScores1 = scheme.encrypt(scores1Arr, n, logp, logq);

            complex<double> *threshVals = new complex<double>[scores1.size()];
            for (int i = 0; i < scores1.size(); i++)
            {
                threshVals[i] = complex<double>(0.7710923973552727, 0.0);
            }

            Ciphertext ciphThresh = scheme.encrypt(threshVals, n, cScores1.logp, cScores1.logq);

            // Perform encrypted comparison
            Ciphertext ciphCompRes;
            double timeTkn, multCount;

            // check if the ciphFusedScores is same as the one you got after addition and division
            // complex<double> *decFusedScores3 = scheme.decrypt(secretKey, ciphFusedScores);


            auto start_time = std::chrono::high_resolution_clock::now();
            fheComp.newCompG(cScores1, ciphThresh, ciphCompRes, scheme, n, 
                           cScores1.logp, cScores1.logq, timeTkn, 1, 1, 1, multCount, secretKey);
            auto end_time = std::chrono::high_resolution_clock::now();

            auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
            avgElapsedTime += elapsed_time;
            avgCpuTime += timeTkn;

            cout << "Time taken for " << batches[bchIdx].size() << " comparisons: " << timeTkn << " ms" << endl;
            cout << "Elapsed time: " << elapsed_time << " ms" << endl;

            // Decrypt results for verification
            complex<double> *decVals = scheme.decrypt(secretKey, ciphCompRes);
                // complex<double> *decFusedScores = scheme.decrypt(secretKey, ciphFusedScores);
            complex<double> *decFusedScores = scheme.decrypt(secretKey, cScores1);
            // Process results
            stringstream buffer;
            for (int row_i = 0; row_i < batches[bchIdx].size(); row_i++)
            {
                if (doneIdx < data.size()) // Only process real data, not dummy rows
                {
                    // Unencrypted fusion: add scores and divide by 2
                    double unencFusedScore = scores1[row_i].real();
                    double unEncDecision = (unencFusedScore >= 0.7710923973552727) ? 1.0 : 0.0;

                    buffer << batches[bchIdx][row_i][0] << "," << batches[bchIdx][row_i][1] << ","
                           << scores1[row_i].real() << ","
                           << batches[bchIdx][row_i][3] << "," << unencFusedScore << ","
                           << unEncDecision << "," << decFusedScores[row_i].real() << ","
                           << decVals[row_i].real() << endl;
                }

                if (row_i > 0 && row_i % 1000 == 0)
                {
                    fusion_file << buffer.rdbuf();
                    buffer.str("");
                    fusion_file.flush();
                }
                doneIdx++;
            }

            if (!buffer.str().empty())
            {
                fusion_file << buffer.rdbuf();
            }

            // Clean up
            delete[] scores1Arr;
            delete[] threshVals;
        }

        cout << "Average CPU time: " << avgCpuTime / batches.size() << " ms" << endl;
        cout << "Average elapsed time: " << avgElapsedTime / batches.size() << " ms" << endl;
        
        fusion_file.close();
        cout << "2-Score fusion (like run_flag 23) completed! Results saved to /home/csgrad/susimmuk/authentication/data/fusion_feature_1_1_1.csv" << endl;
    }

    return 0;
}