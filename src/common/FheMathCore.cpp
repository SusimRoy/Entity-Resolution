// #include "../../libs/HEAAN/src/HEAAN.h"
#include "/home/csgrad/susimmuk/boxtraining/AdaFace/fhe_cpp/ImprovedLT/HEAAN/src/HEAAN.h"
#include "FheMathCore.h"
#include "FheComp.h"
#include "FheRelu.h"
#include "omp.h"

// using namespace heaan;
using namespace NTL;
using namespace std;

void FheMathCore::inverse(
    Ciphertext x,
    Scheme &scheme,
    Ciphertext &result,
    long logp,
    int iter,
    SecretKey secretKey)
{
    Ciphertext minusCiph;
    Ciphertext a;
    Ciphertext b;

    scheme.multByConstAndEqual(x, -1.0, logp);

    scheme.reScaleByAndEqual(x, logp);

    a = scheme.addConst(x, 2.0, logp);
    b = scheme.addConst(x, 1.0, logp);

    Ciphertext onePlusBn;
    for (int i = 0; i < iter; i++)
    {
        b = scheme.square(b);
        scheme.reScaleByAndEqual(b, logp);

        onePlusBn = scheme.addConst(b, 1, logp);
        scheme.modDownToAndEqual(a, onePlusBn.logq);

        scheme.multAndEqual(a, onePlusBn);
        scheme.reScaleByAndEqual(a, logp);
    }
    result = scheme.addConst(a, 0, logp);
}

void FheMathCore::getQPLC(Ciphertext &modIScores,
                       vector<double> genQuantiles,
                       vector<double> impQuantiles,
                       Scheme &scheme,
                       SchemeAlgo &algo,
                       long n,
                       long logp,
                       long logq,
                       SecretKey secretKey)
{

    cout <<"Qplc start "<<modIScores.logp << " " <<modIScores.logq <<endl;

    FheComp fheComp = FheComp();

    vector<Ciphertext> genComps;
    vector<Ciphertext> impComps;
    for (int i = 0; i < genQuantiles.size(); i++)
    {
        Ciphertext compGenVals;
        Ciphertext compImpVals;
        fheComp.compGAgainstConst(
            modIScores,
            genQuantiles[i],
            compGenVals,
            scheme,
            n,
            logp,
            logq,
            1, 1, 1,
            secretKey);

        fheComp.compGAgainstConst(
            modIScores,
            impQuantiles[i],
            compImpVals,
            scheme,
            n,
            logp,
            logq,
            1, 1, 1,
            secretKey);

        genComps.push_back(compGenVals);
        impComps.push_back(compImpVals);

        cout << "Quantile comp "<<i+1<<endl;
    }

    vector<Ciphertext> oneMinusGenComps;
    vector<Ciphertext> oneMinusImpComps;

    for (int i = 0; i < genComps.size(); i++)
    {
        Ciphertext negGenComp;
        negGenComp = scheme.multByConst(genComps[i], -1.0, logp);
        scheme.reScaleByAndEqual(negGenComp, logp);
        scheme.addConstAndEqual(negGenComp, 1.0, logp);
        oneMinusGenComps.push_back(negGenComp);

        Ciphertext negImpComp;
        negImpComp = scheme.multByConst(impComps[i], -1.0, logp);
        scheme.reScaleByAndEqual(negImpComp, logp);
        scheme.addConstAndEqual(negImpComp, 1.0, logp);
        oneMinusImpComps.push_back(negImpComp);
    }

    cout << "One minus "<<endl;

    vector<vector<string>> computedComps;

    for (int i = 0; i < genComps.size(); i++)
    {
        // cout << "Quantile: " << genQuantiles[i] << endl;
        complex<double> *decvals = scheme.decrypt(secretKey, genComps[i]);
        vector<string> computedComp;
        for (int j = 0; j < n; j++)
        {
            computedComp.push_back(to_string(decvals[j].real()));
        }
        cout << endl;
        computedComps.push_back(computedComp);
    }

    std::string compFileName = "/home/csgrad/susimmuk/authentication/data/computed_comps.csv";
    std::ofstream outputFile(compFileName);

    for (const auto &row : computedComps)
    {
        for (size_t i = 0; i < row.size(); ++i)
        {
            // Write each element to the file
            outputFile << row[i];

            // Add a comma to separate values (except for the last element in each row)
            if (i < row.size() - 1)
            {
                outputFile << ",";
            }
        }
        // Add a new line to separate rows
        outputFile << "\n";
    }

    outputFile << "\n";

    outputFile.close();

    std::string oneMinusCompFileName = "/home/csgrad/susimmuk/authentication/data/one_minus_computed_comps.csv";
    std::ofstream oneMinuscompsFile(oneMinusCompFileName);

    for (int i = 0; i < oneMinusGenComps.size(); i++)
    {
        complex<double> *oneMinusDecVals = scheme.decrypt(secretKey, oneMinusGenComps[i]);
        for (int j = 0; j < n; j++)
        {
            oneMinuscompsFile << to_string(oneMinusDecVals[j].real()) << ", ";
        }

        oneMinuscompsFile << "\n";
    }

    oneMinuscompsFile << "\n";

    oneMinuscompsFile.close();

    std::string prodsFileName = "/home/csgrad/susimmuk/authentication/data/prods.csv";
    std::ofstream prodsFile(prodsFileName);

    vector<Ciphertext> genCompProds;
    vector<Ciphertext> impCompProds;

    for (int i = 0; i < genComps.size(); i++)
    {
        Ciphertext genCompProd;
        scheme.modDownByAndEqual(genComps[i], logp);
        genCompProd = scheme.mult(genComps[i], oneMinusGenComps[i]);
        scheme.reScaleByAndEqual(genCompProd, logp);
        complex<double> *prodDec = scheme.decrypt(secretKey, genCompProd);
        for (int j = 0; j < n; j++) {
            prodsFile << to_string(prodDec[j].real()) << ", ";
        }
        prodsFile << "\n";
        genCompProds.push_back(genCompProd);
    }

    cout << genComps[0].logp << " " <<genComps[0].logq <<endl;
    prodsFile << "\n";

    for (int i = 0; i < impComps.size(); i++)
    {
        Ciphertext impCompProd;
        scheme.modDownByAndEqual(impComps[i], logp);
        impCompProd = scheme.mult(impComps[i], oneMinusImpComps[i]);
        scheme.reScaleByAndEqual(impCompProd, logp);
        complex<double> *prodDec = scheme.decrypt(secretKey, impCompProd);
        for (int j = 0; j < n; j++) {
            prodsFile << to_string(prodDec[j].real()) << ", ";
        }
        prodsFile << "\n";
        impCompProds.push_back(impCompProd);
    }

    cout << impComps[0].logp << " " <<impComps[0].logq <<endl;

    prodsFile.close();

    Ciphertext ciphGenMaxIdx;
    getMaxIdx(genCompProds, ciphGenMaxIdx, scheme, algo, n, logp, logq, secretKey);

    Ciphertext ciphImpMaxIdx;
    getMaxIdx(genCompProds, ciphImpMaxIdx, scheme, algo, n, logp, logq, secretKey);

    std::string maxFileName = "/home/csgrad/susimmuk/authentication/data/max_results.csv";
    std::ofstream maxFile(maxFileName);

    complex<double> *maxDecVals = scheme.decrypt(secretKey, ciphGenMaxIdx);
    for(int i=0; i<n; i++) {
        maxFile << to_string(maxDecVals[i].real()) << ", ";
    }
    maxFile << '\n';
    maxFile << '\n';
    complex<double> *maxDecVals1 = scheme.decrypt(secretKey, ciphImpMaxIdx);
    for(int i=0; i<n; i++) {
        maxFile << to_string(maxDecVals1[i].real()) << ", ";
    }

    maxFile.close();
}

void FheMathCore::getMaxIdx(
    vector<Ciphertext> &ciphVec,
    Ciphertext &maxIdx,
    Scheme &scheme,
    SchemeAlgo &algo,
    long n,
    long logp,
    long logq,
    SecretKey secretKey)
{   
    int INVERSE_STEPS = 8;

    if (ciphVec.size() < 2)
    {
        maxIdx = ciphVec[0];
    }
    Ciphertext vecAvg;
    vecAvg = scheme.add(ciphVec[0], ciphVec[1]);
    for (int i = 2; i < ciphVec.size(); i++)
    {
        scheme.addAndEqual(vecAvg, ciphVec[i]);
    }

    scheme.multByConstAndEqual(vecAvg, 1.0 / (ciphVec.size()), logp);
    scheme.reScaleByAndEqual(vecAvg, logp);

    cout << vecAvg.logp << " " << vecAvg.logq;

    Ciphertext avgInverse;
    // inverse(vecAvg, scheme, avgInverse, logp, INVERSE_STEPS, secretKey);
    avgInverse = algo.inverse(vecAvg, logp, INVERSE_STEPS);
    
    cout << "Average"<<endl;
    cout << avgInverse.logp << " " << avgInverse.logq;

    int numCiphers = ciphVec.size();
    vector<Ciphertext> bjs;

    for (int i = 0; i < numCiphers - 1; i++)
    {
        Ciphertext bj;
        bj = scheme.multByConst(ciphVec[i], (1.0 / numCiphers), logp);
        scheme.reScaleByAndEqual(bj, logp);

        for (int j = 0; j < INVERSE_STEPS; j++)
        {
            scheme.modDownByAndEqual(bj, logp);
        }
        scheme.multAndEqual(bj, avgInverse);
        scheme.reScaleByAndEqual(bj, logp);
        bjs.push_back(bj);
    }

    Ciphertext sumBjsNMinusOne;
    sumBjsNMinusOne = scheme.add(bjs[0], bjs[1]);
    for (int i = 2; i < ciphVec.size() - 1; i++)
    {
        scheme.addAndEqual(sumBjsNMinusOne, bjs[i]);
    }

    Ciphertext bn;
    bn = scheme.multByConst(sumBjsNMinusOne, -1.0, logp);
    scheme.reScaleByAndEqual(bn, logp);

    scheme.addConstAndEqual(bn, 1.0, logp);

    for (int i = 0; i < bjs.size(); i++)
    {
        scheme.modDownByAndEqual(bjs[i], logp);
    }

    bjs.push_back(bn);

    for(int i = 0; i < 5; i++)
    {
        for(int j = 0; j < bjs.size(); j++)
        {
            bjs[j] = algo.powerOf2(bjs[j], bjs[j].logp, 1);
        }

        Ciphertext sumBjSq;
        sumBjSq = scheme.add(bjs[0], bjs[1]);
        for (int j = 2; j < bjs.size(); j++)
        {
            scheme.addAndEqual(sumBjSq, bjs[j]);
        }

        Ciphertext invBjSq;
        // inverse(sumBjSq, scheme, invBjSq, logp, INVERSE_STEPS, secretKey);
        invBjSq = algo.inverse(sumBjSq, logp, INVERSE_STEPS);

        for (int j = 0; j < bjs.size(); j++)
        {
            for (int k = 0; k < INVERSE_STEPS; k++)
            {
                scheme.modDownByAndEqual(bjs[j], logp);
            }
        }

        for (int j = 0; j < bjs.size() - 1; j++)
        {
            scheme.multAndEqual(bjs[j], invBjSq);
            scheme.reScaleByAndEqual(bjs[j], logp);
        }

        Ciphertext sumExceptLast;
        sumExceptLast = scheme.add(bjs[0], bjs[1]);

        for (int j = 2; j < bjs.size(); j++)
        {
            scheme.addAndEqual(sumExceptLast, bjs[j]);
        }

        scheme.multByConstAndEqual(sumExceptLast, -1.0, logp);
        scheme.reScaleByAndEqual(sumExceptLast, logp);

        scheme.addConstAndEqual(sumExceptLast, 1.0, logp);

        for (int j = 0; j < bjs.size() - 1; j++)
        {
            scheme.modDownByAndEqual(bjs[j], logp);
        }

        bjs[bjs.size() - 1] = sumExceptLast;
    }

    for(int i=0; i<bjs.size(); i++) {
        scheme.multByConstAndEqual(bjs[i], static_cast<double>(i) + 1.0, logp);
        scheme.reScaleByAndEqual(bjs[i], logp);
    }

    maxIdx = scheme.add(bjs[0], bjs[1]);

    for (int i = 2; i < bjs.size(); i++)
    {
        scheme.addAndEqual(maxIdx, bjs[i]);
    }

    scheme.addConstAndEqual(maxIdx, -1.0, logp);
}