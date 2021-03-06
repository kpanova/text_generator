/****************************************************************************
**
** MIT License
**
** Copyright (c) 2018 Dmitry Bravikov
**
** Permission is hereby granted, free of charge, to any person obtaining a
** copy of this software and associated documentation files (the "Software"),
** to deal in the Software without restriction, including without limitation
** the rights to use, copy, modify, merge, publish, distribute, sublicense,
** and/or sell copies of the Software, and to permit persons to whom the
** Software is furnished to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included
** in all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
** OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
** THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
** FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
** DEALINGS IN THE SOFTWARE.
**
****************************************************************************/

#include "markov_chain.h"

#include <fstream>
#include <iostream>
#include <cstdlib>
#include <ctime>

MarkovChain::MarkovChain(const uint32_t& order)
: m_order(order)
{
    jOrder() = m_order;
}

MarkovChain::MarkovChain(const std::string& filename)
{
    std::ifstream file{filename};

    if (file.fail())
    {
        std::cerr << "Не удалось открыть файл с цепью маркова." << std::endl;
        exit(EXIT_FAILURE);
    }

    try
    {
        file >> m_json;

        file.close();

        const auto order = jOrder().get<decltype(m_order)>();

        if (
            !jOrder().is_number_unsigned()
            || order > std::numeric_limits<decltype(m_order)>::max()
        )
        {
            std::cerr << "Недопустимый порядок цепи Маркова." << std::endl;
            exit(EXIT_FAILURE);
        }

        m_order = order;

        if (jWords().empty())
        {
            std::cerr << "Отсутствуют элементы в цепи Маркова." << std::endl;
            exit(EXIT_FAILURE);
        }
    }
    catch (json::parse_error& e)
    {
        std::cerr
            << "Ошибка при чтении JSON: " << e.what() << std::endl
            << "Идентификатор исключения: " << e.id << std::endl
            << "Номер байта: " << e.byte << std::endl
        ;
        exit(EXIT_FAILURE);
    }
}

void MarkovChain::append(const std::vector<std::string>& words)
{
    if (words.size() < m_order + 2)
    {
        return;
    }

    for (unsigned i1 = 0; i1 < words.size() - m_order - 1; i1++)
    {
        json* jWord = &jWords();

        for (unsigned i2 = 0; i2 < m_order + 1; i2++)
        {
            auto word = words[i1 + i2];
            jWord = &(*jWord)[word];
        }

        json& jsonCount = jNextWordsCount(*jWord);

        if (jsonCount.is_null())
        {
            jsonCount = 1;
        }
        else
        {
            jsonCount = jsonCount.get<unsigned>() + 1;
        }

        json& jsonWord = jNextWords(*jWord)[words[i1 + m_order + 1]];

        if (jsonWord.is_null())
        {
            jsonWord = 1;
        }
        else
        {
            jsonWord = jsonWord.get<unsigned>() + 1;
        }
    }
}

std::string MarkovChain::toText() const
{
    static const int indentSize = 4;
    return m_json.dump(indentSize);
}

uint32_t MarkovChain::order() const
{
    return m_order;
}

std::string MarkovChain::nextWord(const std::vector<std::string>& words)
{
    static bool sranded = false;
    if (!sranded)
    {
        std::srand(unsigned(std::time(0)));
        sranded = true;
    }

    size_t minWordCount = m_order + 1;

    if (words.size() < minWordCount)
    {
        return std::string{};
    }

    json* jWord = &jWords();

    for (auto it = words.end() - minWordCount; it != words.end(); ++it)
    {
        auto w = (*jWord).find(*it);
        if (w == (*jWord).end())
        {
            return std::string{};
        }
        jWord = &(*w);
    }

    const int randomValue = std::rand();

    auto count = jNextWordsCount(*jWord).get<uint32_t>();

    if (count > std::numeric_limits<decltype(randomValue)>::max())
    {
        std::cerr << "Недопустимое количество слов." << std::endl;
        exit(EXIT_FAILURE);
    }

    const uint32_t randomWordPosition = randomValue % count;

    auto jNextWords_ = jNextWords(*jWord);

    uint32_t currentWordPosition = 0;
    for (auto it = jNextWords_.begin(); it != jNextWords_.end(); ++it)
    {
        const uint32_t c = it.value();
        currentWordPosition += c;

        if (currentWordPosition > randomWordPosition)
        {
            return it.key();
        }
    }

    std::cerr << "Здесь оказаться невозможно." << std::endl;
    exit(EXIT_FAILURE);
}
