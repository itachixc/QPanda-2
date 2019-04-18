/*
Copyright (c) 2017-2018 Origin Quantum Computing. All Right Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
#include "Core/QPanda.h"
#include "Utils/Utilities.h"

USING_QPANDA
using namespace std;

QProg Grover(vector<Qubit*> qVec, vector<ClassicalCondition> cVec, int target)
{
    QProg  grover = CreateEmptyQProg();
    QCircuit  init = CreateEmptyCircuit();
    QCircuit  oracle = CreateEmptyCircuit();
    QCircuit  reverse = CreateEmptyCircuit();
    init << H(qVec[0]) << H(qVec[1]) << X(qVec[2]) << H(qVec[2]);
    vector<Qubit *> controlVector;
    controlVector.push_back(qVec[0]);
    controlVector.push_back(qVec[1]);
    //U4  sqrtH(0.5*PI, 0, 0.25*PI, PI);
    QGate  toff = X(qVec[2]);
    toff.setControl(controlVector);
    switch (target)
    {
    case 0:
        oracle << X(qVec[0]) << X(qVec[1]) << toff << X(qVec[0]) << X(qVec[1]);
        break;
    case 1:
        oracle << X(qVec[0]) << toff << X(qVec[0]);
        break;
    case 2:
        oracle << X(qVec[1]) << toff << X(qVec[1]);
        break;
    case 3:
        oracle << toff;
        break;
    }
    reverse << H(qVec[0]) << H(qVec[1]) << X(qVec[0]) << X(qVec[1])
        << H(qVec[1]) << CNOT(qVec[0], qVec[1]);
    reverse << H(qVec[1]) << X(qVec[0]) << X(qVec[1]) << H(qVec[0]) << H(qVec[1]) << X(qVec[2]);
    grover << init << oracle << reverse << Measure(qVec[0], cVec[0]) << Measure(qVec[1], cVec[1]);
    return grover;
}

int main()
{
    int target;
    cout << "input the input function" << endl
        << "The function has a boolean input" << endl
        << "and has a boolean output" << endl
        << "target=(0/1/2/3)?";
    cin >> target;
    cout << "Programming the circuit..." << endl;
    init();

    int qubit_number = 3;
    vector<Qubit*> qv =  qAllocMany(qubit_number);

    int cbitnum = 2;
    vector<ClassicalCondition> cv =  cAllocMany(cbitnum);

    auto groverprog = Grover(qv, cv, target);
    auto resultMap = directlyRun(groverprog);
    if (resultMap["c0"])
    {
        if (resultMap["c1"])
        {
            cout << "target number is 3 !";
        }
        else
        {
            cout << "target number is 2 !";
        }
    }
    else
    {
        if (resultMap["c1"])
        {
            cout << "target number is 1 !";
        }
        else
        {
            cout << "target number is 0 !";
        }
    }
    finalize();
}

