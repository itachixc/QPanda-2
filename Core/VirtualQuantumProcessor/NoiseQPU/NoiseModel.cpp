
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

#include "Core/VirtualQuantumProcessor/NoiseQPU/NoiseModel.h"
#include <complex>
USING_QPANDA
using namespace rapidjson;
using namespace std;
QStat matrix_tensor(const QStat &matrix_left, const QStat &matrix_right)
{
    int size = (int)matrix_left.size();
    QStat matrix_result(size*size, 0);

    for (int i = 0; i < size; i++)
    {
        for (int j = 0; j < size; j++)
        {
            matrix_result[((i >> 1) << 3) + ((j >> 1) << 2) + ((i % 2) << 1) + (j % 2)] = matrix_left[i] * matrix_right[j];
        }
    }
    return matrix_result;
}
bool damping_kraus_operator(Value &value, NoiseOp & noise)
{
    if ((!value.IsArray()) || (value.Size()!=2))
    {
        QCERR("param error");
        throw std::invalid_argument("param error");
    }
    if (NOISE_MODEL::DAMPING_KRAUS_OPERATOR != (NOISE_MODEL)value[0].GetUint())
    {
        QCERR("param error");
        throw std::invalid_argument("param error");
    }

    if (!value[1].IsDouble())
    {
        QCERR("param error");
        throw std::invalid_argument("param error");
    }

    double probability = value[1].GetDouble();

    noise.resize(2);
    noise[0] = { 1,0,0,sqrt(1 - probability) };
    noise[1] = { 0,sqrt(probability),0,0 };
    return 1;
}
bool dephasing_kraus_operator(Value & value, NoiseOp & noise)
{
    if ((!value.IsArray()) || (value.Size()!=2))
    {
        QCERR("param error");
        throw std::invalid_argument("param error");
    }
    if (NOISE_MODEL::DEPHASING_KRAUS_OPERATOR != (NOISE_MODEL)value[0].GetUint())
    {
        QCERR("param error");
        throw std::invalid_argument("param error");
    }

    if (!value[1].IsDouble())
    {
        QCERR("param error");
        throw std::invalid_argument("param error");
    }
    double probability = value[1].GetDouble();
    noise.resize(2);
    noise[0] = { sqrt(1 - probability),0,0,sqrt(1 - probability) };
    noise[1] = { sqrt(probability),0,0,-sqrt(probability) };
    return 1;
}
//1 - np.exp(-float(gate_time) / float(T1))
//gamma_phi -= float(gate_time) / float(2 * T1)
//p=.5 * (1 - np.exp(-2 * gamma_phi))
//bool decoherence_kraus_operator(double t1, double t2, double gate_time, NoiseOp & noise)
bool decoherence_kraus_operator(Value & value, NoiseOp & noise)
{
    if ((!value.IsArray())||(value.Size()!=4))
    {
        QCERR("param error");
        throw std::invalid_argument("param error");
    }
    if (NOISE_MODEL::DECOHERENCE_KRAUS_OPERATOR != (NOISE_MODEL)value[0].GetUint())
    {
        QCERR("param error");
        throw std::invalid_argument("param error");
    }

    for (SizeType i = 1; i < value.Size(); i++)
    {
        if (!value[i].IsDouble())
        {
            QCERR("param error");
            throw std::invalid_argument("param error");
        }
    }

    double t1 = value[1].GetDouble();
    double t2 = value[2].GetDouble();
    double gate_time = value[3].GetDouble();

    Document document;
    document.SetObject();
    auto & alloc =document.GetAllocator();
    NoiseOp damping, dephasing;

    Value damping_value(kArrayType);
    damping_value.PushBack(NOISE_MODEL::DAMPING_KRAUS_OPERATOR,alloc);
    damping_value.PushBack(1 - exp(-gate_time / t1), alloc);
    damping_kraus_operator(damping_value, damping);
    double gamma_phi = gate_time / t2;
    gamma_phi -= gate_time / (2 * t1);

    Value dephasing_value(kArrayType);
    dephasing_value.PushBack(NOISE_MODEL::DEPHASING_KRAUS_OPERATOR, alloc);
    dephasing_value.PushBack(0.5 * (1 - exp(-2 * gamma_phi)), alloc);
    dephasing_kraus_operator(dephasing_value, dephasing);

    for (auto iter : damping)
    {
        for (auto iter1 : dephasing)
        {
            noise.push_back(matrix_multiply(iter, iter1));
        }
    }
    return 1;
}

bool double_damping_kraus_operator(Value & value, NoiseOp & noise)
{
    NoiseOp ntemp;
    if ((!value.IsArray()) || (value.Size() != 2))
    {
        QCERR("param error");
        throw std::invalid_argument("param error");
    }
    if (NOISE_MODEL::DOUBLE_DAMPING_KRAUS_OPERATOR != (NOISE_MODEL)value[0].GetUint())
    {
        QCERR("param error");
        throw std::invalid_argument("param error");
    }

    if (!value[1].IsDouble())
    {
        QCERR("param error");
        throw std::invalid_argument("param error");
    }

    double probability = value[1].GetDouble();

    ntemp.resize(2);
    ntemp[0] = { 1,0,0,sqrt(1 - probability) };
    ntemp[1] = { 0,sqrt(probability),0,0 };
    for (auto i = 0; i < ntemp.size(); i++)
    {
        for (auto j = 0; j < ntemp.size(); j++)
        {
            noise.push_back(matrix_tensor(ntemp[i], ntemp[j]));
        }
    }
    return 1;
}

bool double_dephasing_kraus_operator(Value & value, NoiseOp & noise)
{
    NoiseOp ntemp;
    if ((!value.IsArray()) || (value.Size() != 2))
    {
        QCERR("param error");
        throw std::invalid_argument("param error");
    }
    if (NOISE_MODEL::DOUBLE_DEPHASING_KRAUS_OPERATOR != (NOISE_MODEL)value[0].GetUint())
    {
        QCERR("param error");
        throw std::invalid_argument("param error");
    }

    if (!value[1].IsDouble())
    {
        QCERR("param error");
        throw std::invalid_argument("param error");
    }
    double probability = value[1].GetDouble();
    ntemp.resize(2);
    ntemp[0] = { sqrt(1 - probability),0,0,sqrt(1 - probability) };
    ntemp[1] = { sqrt(probability),0,0,-sqrt(probability) };
    for (auto i = 0; i < ntemp.size(); i++)
    {
        for (auto j = 0; j < ntemp.size(); j++)
        {
            noise.push_back(matrix_tensor(ntemp[i], ntemp[j]));
        }
    }
    return 1;
}

bool double_decoherence_kraus_operator(Value & value, NoiseOp & noise)
{
    NoiseOp ntemp;
    if ((!value.IsArray()) || (value.Size() != 4))
    {
        QCERR("param error");
        throw std::invalid_argument("param error");
    }
    if (NOISE_MODEL::DOUBLE_DECOHERENCE_KRAUS_OPERATOR != (NOISE_MODEL)value[0].GetUint())
    {
        QCERR("param error");
        throw std::invalid_argument("param error");
    }

    for (SizeType i = 1; i < value.Size(); i++)
    {
        if (!value[i].IsDouble())
        {
            QCERR("param error");
            throw std::invalid_argument("param error");
        }
    }

    double t1 = value[1].GetDouble();
    double t2 = value[2].GetDouble();
    double gate_time = value[3].GetDouble();

    Document document;
    document.SetObject();
    auto & alloc = document.GetAllocator();
    NoiseOp damping, dephasing;

    Value damping_value(kArrayType);
    damping_value.PushBack(NOISE_MODEL::DAMPING_KRAUS_OPERATOR, alloc);
    damping_value.PushBack(1 - exp(-gate_time / t1), alloc);
    damping_kraus_operator(damping_value, damping);
    double gamma_phi = gate_time / t2;
    gamma_phi -= gate_time / (2 * t1);

    Value dephasing_value(kArrayType);
    dephasing_value.PushBack(NOISE_MODEL::DEPHASING_KRAUS_OPERATOR, alloc);
    dephasing_value.PushBack(0.5 * (1 - exp(-2 * gamma_phi)), alloc);
    dephasing_kraus_operator(dephasing_value, dephasing);

    for (auto iter : damping)
    {
        for (auto iter1 : dephasing)
        {
            ntemp.push_back(matrix_multiply(iter, iter1));
        }
    }
    for (auto i = 0; i < ntemp.size(); i++)
    {
        for (auto j = 0; j < ntemp.size(); j++)
        {
            noise.push_back(matrix_tensor(ntemp[i], ntemp[j]));
        }
    }
    return 1;
}

bool pauli_kraus_map(Value & value, NoiseOp & noise)
{
    if ((!value.IsArray()) || (value.Size() != 5))
    {
        QCERR("param error");
        throw std::invalid_argument("param error");
    }
    if (NOISE_MODEL::PAULI_KRAUS_MAP != (NOISE_MODEL)value[0].GetUint())
    {
        QCERR("param error");
        throw std::invalid_argument("param error");
    }
    vector<double> probability;
    for (SizeType i = 1; i < value.Size(); i++)
    {
        if (!value[i].IsDouble())
        {
            QCERR("param error");
            throw std::invalid_argument("param error");
        }
        else
        {
            probability.push_back(value.GetDouble());
        }
    }
    QStat PAULI_I = { 1,0,0,1 };
    QStat PAULI_X = { 0,1,1,0 };
    QStat PAULI_Y = {0,qcomplex_t(0,-1),qcomplex_t(0,-1),0 };
    QStat PAULI_Z = { 1,0,0,-1 };
    if (probability.size() == 4)
    {
        noise.resize(4);
        noise[0] = { sqrt(probability[0]),0,0,sqrt(probability[0]) };
        noise[1] = { 0,sqrt(probability[1]),sqrt(probability[1]),0 };
        noise[2] = { 0,sqrt(probability[2])*qcomplex_t(0,-1),sqrt(probability[2])*qcomplex_t(0,-1),0 };
        noise[3] = { 1,0,0,-sqrt(probability[3]) };
    }
    else if (probability.size() == 16)
    {
        noise.resize(16);
        noise[0] = matrix_tensor({ sqrt(probability[0]),0,0,sqrt(probability[0]) }, { 1,0,0,1 });
        noise[1] = matrix_tensor({ sqrt(probability[1]),0,0,sqrt(probability[1]) }, { 0,1,1,0 });
        noise[2] = matrix_tensor({ sqrt(probability[2]),0,0,sqrt(probability[2]) }, { 0,qcomplex_t(0,-1),qcomplex_t(0,-1),0 });
        noise[3] = matrix_tensor({ sqrt(probability[3]),0,0,sqrt(probability[3]) }, { 1,0,0,-1 });
        noise[4] = matrix_tensor({ 0, sqrt(probability[4]),sqrt(probability[4]),0 }, { 1,0,0,1 });
        noise[5] = matrix_tensor({ 0, sqrt(probability[5]),sqrt(probability[5]),0 }, { 0,1,1,0 });
        noise[6] = matrix_tensor({ 0, sqrt(probability[6]),sqrt(probability[6]),0 }, { 0,qcomplex_t(0,-1),qcomplex_t(0,-1),0 });
        noise[7] = matrix_tensor({ 0, sqrt(probability[7]),sqrt(probability[7]),0 }, { 1,0,0,-1 });
        noise[8] = matrix_tensor({ 0,sqrt(probability[8])*qcomplex_t(0,-1),sqrt(probability[8])*qcomplex_t(0,-1),0 }, { 1,0,0,1 });
        noise[9] = matrix_tensor({ 0,sqrt(probability[9])*qcomplex_t(0,-1),sqrt(probability[9])*qcomplex_t(0,-1),0 }, { 0,1,1,0 });
        noise[10] = matrix_tensor({ 0,sqrt(probability[10])*qcomplex_t(0,-1),sqrt(probability[10])*qcomplex_t(0,-1),0 },
            { 0,qcomplex_t(0,-1),qcomplex_t(0,-1),0 });
        noise[11] = matrix_tensor({ 0,sqrt(probability[11])*qcomplex_t(0,-1),sqrt(probability[11])*qcomplex_t(0,-1),0 }, { 1,0,0,-1 });
        noise[12] = matrix_tensor({ sqrt(probability[12]),0,0,-sqrt(probability[12]) }, { 1,0,0,1 });
        noise[13] = matrix_tensor({ sqrt(probability[13]),0,0,-sqrt(probability[13]) }, { 0,1,1,0 });
        noise[14] = matrix_tensor({ sqrt(probability[14]),0,0,-sqrt(probability[14]) }, { 0,qcomplex_t(0,-1),qcomplex_t(0,-1),0 });
        noise[15] = matrix_tensor({ sqrt(probability[15]),0,0,-sqrt(probability[15]) }, { 1,0,0,-1 });
    }
    return 1;
}

NoiseModeMap & NoiseModeMap::getInstance()
{
    static NoiseModeMap map;
    return map;
}

noise_mode_function NoiseModeMap::operator[](NOISE_MODEL type)
{
    auto iter = m_function_map.find(type);
    if (iter == m_function_map.end())
    {
        QCERR("noise model type error");
        throw invalid_argument("noise model type error");
    }

    return iter->second;
}

NoiseModeMap::NoiseModeMap()
{
    m_function_map.insert(make_pair(DAMPING_KRAUS_OPERATOR, damping_kraus_operator));
    m_function_map.insert(make_pair(DEPHASING_KRAUS_OPERATOR, dephasing_kraus_operator));
    m_function_map.insert(make_pair(DECOHERENCE_KRAUS_OPERATOR, decoherence_kraus_operator));
    m_function_map.insert(make_pair(PAULI_KRAUS_MAP, pauli_kraus_map));
    m_function_map.insert(make_pair(DOUBLE_DAMPING_KRAUS_OPERATOR, double_damping_kraus_operator));
    m_function_map.insert(make_pair(DOUBLE_DEPHASING_KRAUS_OPERATOR, double_dephasing_kraus_operator));
    m_function_map.insert(make_pair(DOUBLE_DECOHERENCE_KRAUS_OPERATOR, double_decoherence_kraus_operator));
}