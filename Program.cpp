#include <iostream>
#include <cstdlib>
#include <utility>
#include <vector>
#include <string>
#include <fstream>
#include <map>
#include <cmath>

class Bucket {
private:
    static const short BUCKET_SIZE = 4;
    unsigned char bucket[BUCKET_SIZE];
public:

    void setItem(int index, int value) { bucket[index] = value; }

    int getItem(int index) { return bucket[index]; }

    bool insert(unsigned char el) {
        for (int i = 1; i < BUCKET_SIZE; i++) {
            if (bucket[i] == 0) {
                bucket[i] = el;
                return true;
            }
        }
        return false;
    };

    int getFingerprintIndex(unsigned char el) {
        for (int i = 0; i < BUCKET_SIZE; i++) {
            if (bucket[i] == el) {
                return i;
            }
        }
        return -1;
    };
};

class CuckooFilter {
private:
    // Количество групп элементов(бакетов) в фильтре
    unsigned int bucketsLength = 0;
    // Массив бакетов
    Bucket *buckets;
    static const short MAX_NUM_KICKS = 500;
public:
    explicit CuckooFilter(unsigned int maxCuckooCount) {
        bucketsLength = (int) ceil(maxCuckooCount * 1.06);
        buckets = new Bucket[bucketsLength];
    };

    explicit CuckooFilter()
    = default;;

    bool lookUp(std::string data) {
        int32_t hashData = MyHash(std::move(data));
        unsigned char fingerPrint = getFingerprint(hashData);
        unsigned int i1 = (unsigned int) abs((long) (std::hash<int32_t>()(hashData) % bucketsLength));
        unsigned int i2 = (unsigned int) abs((long) (i1 ^ (std::hash<unsigned char>()(fingerPrint) % bucketsLength)));
        //if bucket[i1] or bucket[i2] has fingerPrint then return True
        if (buckets[i1].getFingerprintIndex(fingerPrint) > -1) {
            return true;
        }
        if (buckets[i2].getFingerprintIndex(fingerPrint) > -1) {
            return true;
        }
        return false;
    }

    bool insert(const std::string &data) {
        // Если элемент уже установлен
        if (lookUp(data)) {
            return false;
        }
        int32_t hashData = MyHash(data);
        unsigned char fingerPrint = getFingerprint(hashData);
        unsigned int i1 = (unsigned int) abs((long) (std::hash<int32_t>()(hashData) % bucketsLength));
        unsigned int i2 = (unsigned int) abs((long) (i1 ^ (std::hash<unsigned char>()(fingerPrint) % bucketsLength)));
        //if bucket[i1] or bucket[i2] has an empty entry then add fingerPrint to that bucket;
        if (insertToBucket(fingerPrint, i1) || insertToBucket(fingerPrint, i2)) {
            return true;
        }
        //must relocate existing items
        return insertWithRelocation(fingerPrint, randIndex(i1, i2));
    }

    bool insertToBucket(unsigned char fingerPrint, unsigned int i) {
        if (buckets[i].insert(fingerPrint)) {
            return true;
        }
        return false;
    }

    bool insertWithRelocation(unsigned char fingerPrint, unsigned int i) {
        int randomlySelectedEl;
        for (unsigned int j = 0; j < MAX_NUM_KICKS; j++) {
            randomlySelectedEl = rand() % 4;
            unsigned char oldFingerPrint = fingerPrint;
            fingerPrint = buckets[i].getItem(randomlySelectedEl);
            buckets[i].setItem(randomlySelectedEl, oldFingerPrint);
            i = (unsigned int) abs((long) (i ^ (std::hash<unsigned char>()(fingerPrint) % bucketsLength)));
            if (insertToBucket(fingerPrint, i)) {
                return true;
            }
        }
        return false;
    }

    static unsigned char getFingerprint(int32_t data) {
        unsigned char res = data % 255;
        if (res == 0) {
            return res + 1;
        }
        return res;
    }

    static unsigned int randIndex(unsigned int i1, unsigned int i2) {
        // rand() % 2 - генерация случайного числа в диапозоне [0, 1]
        if (rand() % 2 == 1) {
            return i1;
        }
        return i2;
    }

    // Реализация хэширования найдена в интернете.
    static int32_t MyHash(std::string value) {
        unsigned int num = 5381;
        unsigned int num2 = num;
        for (int i = 0; i < value.size(); i += 2) {
            num = (((num << 5) + num) ^ value[i]);

            if (i + 1 < value.size())
                num2 = (((num2 << 5) + num2) ^ value[i + 1]);
        }
        return num + num2 * 1566083941;
    }
};

/// Проверяет, существует ли файл
static bool fileExist(const std::string &name) {
    std::ifstream f(name.c_str());
    return f.good();
}

/// Позволяет работать как с относительными, так и с абсолютными путями
static std::string path(std::string pathStr) {
    if (fileExist(pathStr))
        return pathStr;
    if (fileExist("input/" + pathStr))
        return "input/" + pathStr;
    if (fileExist("output/" + pathStr))
        return "output/" + pathStr;
    throw std::invalid_argument("Такого файла не существуеть!");
}

/// Вспомогательный метод к методу split
static bool space(char c) {
    return std::isspace(c);
}

/// Вспомогательный метод к методу split
static bool notSpace(char c) {
    return !std::isspace(c);
}

/// Метод, иммитирующий split в c#. Разделяет строку на слова и помещает их в вектор.
static std::vector<std::string> split(const std::string &s) {
    typedef std::string::const_iterator iter;
    std::vector<std::string> ret;
    iter i = s.begin();
    while (i != s.end()) {
        i = std::find_if(i, s.end(), notSpace); // find the beginning of a word
        iter j = std::find_if(i, s.end(), space); // find the end of the same word
        if (i != s.end()) {
            ret.emplace_back(i, j);
            i = j;
        }
    }
    return ret;
}

/// Считываем первую команду(кол-во видео на хостинге, например, "videos 6")
static unsigned int amountOfVideos(std::ifstream &fileInput, std::vector<std::string> &output) {
    // Строка для чтения строк из файла
    std::string s;
    unsigned int size = 0;
    if (getline(fileInput, s)) {
        size = (unsigned int) std::stoi(split(s)[1]);
        output.emplace_back("Ok");
    }
    return size;
}

/// Обработка команды типа watch
static void watch(std::vector<std::string> &output, unsigned int size, std::map<std::string, CuckooFilter> &userFilterPairs,
                  std::vector<std::string> input) {
    std::map<std::string, CuckooFilter>::iterator it;
    it = userFilterPairs.find(input[1]);
    // Если пользователь уже был добавлен в словарь userFilterPairs
    if (!(it == userFilterPairs.end())) {
        userFilterPairs[input[1]].insert(input[2]);
        output.emplace_back("Ok");
    } else {
        // Если пользователь не был добавлен в словарь userFilterPairs - добавляем
        userFilterPairs[input[1]] = *new CuckooFilter(size);
        userFilterPairs[input[1]].insert(input[2]);
        output.emplace_back("Ok");
    }
}

/// Обработка команды типа check
static void check(std::vector<std::string> &output, std::map<std::string, CuckooFilter> &userFilterPairs,
                  std::vector<std::string> input) {
    // Если поступившая команда - check
    std::map<std::string, CuckooFilter>::iterator it;
    it = userFilterPairs.find(input[1]);
    // Если пользователь уже был добавлен в словарь userFilterPairs
    if (!(it == userFilterPairs.end())) {
        bool checkRes = userFilterPairs[input[1]].lookUp(input[2]);
        if (checkRes) {
            output.emplace_back("Probably");
        } else {
            output.emplace_back("No");
        }
    } else {
        // Если пользователь не был добавлен в словарь userFilterPairs - очевидно, ответ "No"
        output.emplace_back("No");
    }


}

/// Обработка команд
static void commandProcessing(std::vector<std::string> &output, std::string pathStr) {
    std::ifstream fileInput(path(std::move(pathStr)));
    // Кол-во видео на хостинге
    unsigned int size = amountOfVideos(fileInput, output);
    // Строка для чтения строк из файла
    std::string s;
    // Словарь содержащий пары user# и фильтр с просмотренными им видео
    std::map<std::string, CuckooFilter> userFilterPairs;
    while (getline(fileInput, s)) {
        // Строка команды, например "watch user1 video1",
        // представленная в виде коллекции слов
        std::vector<std::string> input = split(s);
        if (input[0] == "watch") {
            watch(output, size, userFilterPairs, input);
        } else {
            check(output, userFilterPairs, input);
        }
    }
    fileInput.close();
}

/// Вывод выходных данных в файл
static void outputToFile(const std::vector<std::string> &output, std::string pathStr) {
    std::ofstream fileOutput;
    fileOutput.open(path(std::move(pathStr)));
    if (fileOutput.is_open()) {
        for (auto &i : output) {
            fileOutput << i << std::endl;
        }
    }
}

int main(int argc, char *argv[]) {
    // Выходные данные
    std::vector<std::string> output;
    // Обрабатываем поступившие команды
    commandProcessing(output, argv[1]);
    outputToFile(output, argv[2]);
    return 0;
}