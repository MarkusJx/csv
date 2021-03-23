#include <iostream>
#include <fstream>
#include <random>
#include <gtest/gtest.h>

#include "csv.hpp"

#define CSV_ASSERT_NUM_MATCH() ASSERT_EQ(csv.numElements(), (index + 1) * numValues)

#define CSV_CLEAR() CSV_ASSERT_NUM_MATCH();\
                    csv[index].clear();\
                    ASSERT_TRUE(csv[index].empty())

#define CSV_FOR for (int i = 0; i < numValues; i++)

class CSVTestBase : public ::testing::Test {
protected:
    CSVTestBase()
            : r(), e1(r()), int_dist(-10000000, 10000000), double_dist(-10000000.0, 10000000.0), bool_dist(0, 1) {};

    bool getRandomBool() {
        return bool_dist(e1);
    }

    int getRandomInt() {
        return int_dist(e1);
    }

    double getRandomDouble() {
        return double_dist(e1);
    }

    // Source: https://stackoverflow.com/a/24586587
    static std::string getRandomString(std::string::size_type length) {
        static auto &chrs = "0123456789"
                            "abcdefghijklmnopqrstuvwxyz"
                            "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

        thread_local static std::mt19937 rg{std::random_device{}()};
        thread_local static std::uniform_int_distribution<std::string::size_type> pick(0, sizeof(chrs) - 2);

        std::string s;
        s.reserve(length);

        while (length--)
            s += chrs[pick(rg)];

        return s;
    }

private:
    std::random_device r;
    std::default_random_engine e1;

    std::uniform_int_distribution<int> int_dist;
    std::uniform_real_distribution<double> double_dist;
    std::uniform_int_distribution<int> bool_dist;
};

const int numValues = 10000;

class CSVTest : public CSVTestBase {
protected:
    static markusjx::csv csv;

    template<class T>
    static void assert_eq(int index, int i, T cur) {
        if (csv[index][i].as<T>() != cur) {
            if constexpr (std::is_same_v<T, std::string>) {
                ASSERT_EQ(csv[index][i].as<T>(), cur) << "raw value: " << csv[index][i].rawValue();
            } else {
                ASSERT_EQ(std::to_string(csv[index][i].as<T>()), std::to_string(cur))
                                            << "raw value: " << csv[index][i].rawValue();
            }
        } else {
            ASSERT_EQ(csv[index][i].as<T>(), cur) << "raw value: " << csv[index][i].rawValue();
        }
    }

    template<class T>
    static void csv_test(const int index, const std::function<T()> &curGetter) {
        CSV_FOR {
            const T cur = curGetter();
            csv[index][i] = cur;

            assert_eq(index, i, cur);
        }

        CSV_CLEAR();

        CSV_FOR {
            const T cur = curGetter();
            csv.push(cur);

            assert_eq(index, i, cur);
        }

        CSV_CLEAR();

        CSV_FOR {
            const T cur = curGetter();
            csv += cur;

            assert_eq(index, i, cur);
        }

        CSV_ASSERT_NUM_MATCH();

        csv.endline();
    }
};

markusjx::csv CSVTest::csv;

TEST_F(CSVTest, emptyCheck) {
    ASSERT_TRUE(csv.empty());
    ASSERT_EQ(csv.size(), 0);
    ASSERT_EQ(csv.numElements(), 0);
}

TEST_F(CSVTest, integerTest) {
    csv_test<int>(0, [this] { return getRandomInt(); });;
}

TEST_F(CSVTest, longTest) {
    csv_test<long>(1, [this] { return getRandomInt(); });
}

TEST_F(CSVTest, doubleTest) {
    csv_test<double>(2, [this] { return getRandomDouble(); });
}

TEST_F(CSVTest, longDoubleTest) {
    csv_test<long double>(3, [this] { return getRandomDouble(); });
}

TEST_F(CSVTest, floatTest) {
    csv_test<float>(4, [this] { return static_cast<float>(getRandomDouble()); });
}

TEST_F(CSVTest, stringTest) {
    csv_test<std::string>(5, [] { return getRandomString(40);});
}

TEST_F(CSVTest, boolTest) {
    csv_test<bool>(6, [this] { return getRandomBool(); });
}

TEST_F(CSVTest, charTest) {
    csv_test<char>(7, [this] { return static_cast<char>(getRandomInt()); });
}

class ConstructorTest : public CSVTestBase {
};

TEST_F(ConstructorTest, fromVectorTest) {
    std::vector<markusjx::csvrowcolumn<std::string>> data;
    for (int i = 0; i < 100; i++) {
        data.emplace_back(getRandomInt());
        data.emplace_back(getRandomBool());
        data.emplace_back(getRandomDouble());
        data.emplace_back(std::to_string(getRandomDouble()));
    }

    markusjx::csv csv = data;
    EXPECT_EQ(csv.numElements(), 400);
}

TEST_F(ConstructorTest, fromInitializerListTest) {
    markusjx::csv csv = {{"abc", 1,  5,    'd',   false},
                         {25,    42, true, "def", nullptr, "ye"}};
    EXPECT_EQ(csv.numElements(), 11);
}

TEST_F(ConstructorTest, appendTest) {
    markusjx::csv csv1 = {{"abc", 1,  5,    'd',   false},
                         {25,    42, true, "def", nullptr, "ye"}};
    markusjx::csv csv2 = {{"abc", 1,  5,    'd',   false},
                          {25,    42, true, "def", nullptr, "ye"}};

    csv1 << csv2;

    EXPECT_EQ(csv1.numElements(), 22);
}

class EqualityTest : public CSVTestBase {
};

TEST_F(EqualityTest, equalsTest) {
    markusjx::csv csv_1;
    markusjx::csv csv_2;
    for (int i = 0; i < 100; i++) {
        int x = getRandomInt();
        double y = getRandomDouble();
        std::string z = getRandomString(40);

        csv_1 << x << y << z << markusjx::csv::endl;
        csv_2 << x << y << z << markusjx::csv::endl;
    }

    EXPECT_EQ(csv_1, csv_2);
}

TEST_F(EqualityTest, unequalTest) {
    markusjx::csv csv_1, csv_2;
    csv_1 << 1;
    csv_2 << 2;

    EXPECT_NE(csv_1, csv_2);

    csv_1[0][0] = "1";
    EXPECT_NE(csv_1, csv_2);

    csv_1[0][0] = 1.0;
    csv_2[0][0] = 1;
    EXPECT_NE(csv_1, csv_2);
}

TEST_F(EqualityTest, spaceshipTest) {
    markusjx::csv csv;
    for (int i = 0; i < numValues; i++) {
        int n1 = getRandomInt();
        int n2 = getRandomInt();
        csv[0][0] = n1;

        if (n1 > n2) {
            EXPECT_GT(csv[0][0], n2);
        } else if (n1 == n2) {
            EXPECT_EQ(csv[0][0], n2);
        } else {
            EXPECT_LT(csv[0][0], n2);
        }

        if (n1 >= n2) {
            EXPECT_GE(csv[0][0], n2);
        } else if (n1 <= n2) {
            EXPECT_LE(csv[0][0], n2);
        }
    }
}

class NumberTest : public CSVTestBase {
};

TEST_F(NumberTest, incrementTest) {
    markusjx::csv csv;

    for (int i = 0; i < 100; i++) {
        int n = std::abs(getRandomInt() % 1000);
        for (csv[0][0] = 0; csv[0][0] < n; csv[0][0]++);

        ASSERT_EQ(csv[0][0], n);
    }
}

TEST_F(NumberTest, decrementTest) {
    markusjx::csv csv;

    for (int i = 0; i < 100; i++) {
        int n = std::abs(getRandomInt() % 1000);
        for (csv[0][0] = n; csv[0][0] > 0; csv[0][0]--);

        ASSERT_EQ(csv[0][0], 0);
    }
}

TEST_F(NumberTest, addTest) {
    for (int i = 0; i < numValues; i++) {
        int n1 = getRandomInt();
        int n2 = getRandomInt();

        markusjx::csv csv = {n1};
        markusjx::csv _csv = {csv[0][0] + n2};

        ASSERT_EQ(_csv[0][0], n1 + n2);
    }
}

TEST_F(NumberTest, subTest) {
    for (int i = 0; i < numValues; i++) {
        int n1 = getRandomInt();
        int n2 = getRandomInt();

        markusjx::csv csv = {n1};
        markusjx::csv _csv = {csv[0][0] - n2};

        ASSERT_EQ(_csv[0][0], n1 - n2);
    }
}

TEST_F(NumberTest, mulTest) {
    for (int i = 0; i < numValues; i++) {
        int n1 = getRandomInt();
        int n2 = getRandomInt();

        markusjx::csv csv = {n1};
        markusjx::csv _csv = {csv[0][0] * n2};

        ASSERT_EQ(_csv[0][0], n1 * n2);
    }
}

TEST_F(NumberTest, divTest) {
    for (int i = 0; i < numValues; i++) {
        int n1 = getRandomInt();
        int n2 = getRandomInt();

        markusjx::csv csv = {n1};
        markusjx::csv _csv = {csv[0][0] / n2};

        ASSERT_EQ(_csv[0][0], n1 / n2);
    }
}

TEST_F(NumberTest, addAssignTest) {
    for (int i = 0; i < numValues; i++) {
        int n1 = getRandomInt();
        int n2 = getRandomInt();

        markusjx::csv csv = {n1};
        csv[0][0] += n2;

        ASSERT_EQ(csv[0][0], n1 + n2);
    }
}

TEST_F(NumberTest, subAssignTest) {
    for (int i = 0; i < numValues; i++) {
        int n1 = getRandomInt();
        int n2 = getRandomInt();

        markusjx::csv csv = {n1};
        csv[0][0] -= n2;

        ASSERT_EQ(csv[0][0], n1 - n2);
    }
}

TEST_F(NumberTest, mulAssignTest) {
    for (int i = 0; i < numValues; i++) {
        int n1 = getRandomInt();
        int n2 = getRandomInt();

        markusjx::csv csv = {n1};
        csv[0][0] *= n2;

        ASSERT_EQ(csv[0][0], n1 * n2);
    }
}

TEST_F(NumberTest, divAssignTest) {
    for (int i = 0; i < numValues; i++) {
        int n1 = getRandomInt();
        int n2 = getRandomInt();

        markusjx::csv csv = {n1};
        csv[0][0] /= n2;

        ASSERT_EQ(csv[0][0], n1 / n2);
    }
}

class ObjectOperationsTest : public CSVTestBase {
};

TEST_F(ObjectOperationsTest, addTest) {
    for (int i = 0; i < numValues; i++) {
        int n1 = getRandomInt();
        int n2 = getRandomInt();

        markusjx::csv c1 = {n1};
        markusjx::csv c2 = {n2};

        ASSERT_EQ(c1[0][0] + c2[0][0], n1 + n2);
    }
}

TEST_F(ObjectOperationsTest, subTest) {
    for (int i = 0; i < numValues; i++) {
        int n1 = getRandomInt();
        int n2 = getRandomInt();

        markusjx::csv c1 = {n1};
        markusjx::csv c2 = {n2};

        ASSERT_EQ(c1[0][0] - c2[0][0], n1 - n2);
    }
}

TEST_F(ObjectOperationsTest, mulTest) {
    for (int i = 0; i < numValues; i++) {
        long long n1 = getRandomInt();
        long long n2 = getRandomInt();

        markusjx::csv c1 = {n1};
        markusjx::csv c2 = {n2};

        ASSERT_EQ(c1[0][0] * c2[0][0], n1 * n2);
    }
}

TEST_F(ObjectOperationsTest, divTest) {
    for (int i = 0; i < numValues; i++) {
        int n1 = getRandomInt();
        int n2 = getRandomInt();

        markusjx::csv c1 = {n1};
        markusjx::csv c2 = {n2};

        ASSERT_EQ(c1[0][0] / c2[0][0], n1 / n2);
    }
}

class FileTest : public CSVTestBase {
protected:
    static markusjx::csv csv;
};

markusjx::csv FileTest::csv = {{"abc", 1,  5,    'd',   false},
                               {25,    42, true, "def", nullptr, "ye"}};

TEST_F(FileTest, writeTest) {
    std::ofstream ofs("test.csv");
    ofs << csv;
    ofs.close();
}

TEST_F(FileTest, readTest) {
    markusjx::csv c;
    std::ifstream ifs("test.csv");
    ifs >> c;

    EXPECT_EQ(c, csv);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}