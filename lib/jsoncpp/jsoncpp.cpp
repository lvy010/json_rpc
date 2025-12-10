#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include "jsoncpp/json/json.h"

using namespace std;

// 序列化
void serialize(string& str)
{
    const char* name = "小明";
    int age = 18;
    const char* sex = "男";
    float scores[3] = { 88, 77.5, 66 };
    
    Json::Value stu;
    stu["name"] = name;
    stu["age"] = age;
    stu["sex"] = sex;
    for (auto score : scores)
        stu["score"].append(score);

    Json::StyledStreamWriter ssw;
    cout << "StyledStreamWriter: " << endl;
    ssw.write(cout, stu);
    cout << endl;

    Json::StyledWriter sw;
    cout << "StyledWriter: " << endl;
    cout << sw.write(stu);
    cout << endl;

    Json::FastWriter fw;
    cout << "FastWriter: " << endl;
    cout << fw.write(stu);
    cout << endl;

    Json::StreamWriterBuilder swb; // 工厂类
    unique_ptr<Json::StreamWriter> psw(swb.newStreamWriter()); // RAII管理资源释放
    cout << "StreamWriter: " << endl;
    int ret = psw->write(stu, &cout);
    cout << endl;

    if (ret != 0)
    {
        perror("jsoncpp 序列化错误!\n");
        exit(-1);
    }

    str = fw.write(stu);
}

// 反序列化
void deserialize(const string& str)
{
    Json::Value root1;
    Json::Reader rd;
    rd.parse(str, root1, true);


    Json::Value root2;
    Json::CharReaderBuilder crb;
    unique_ptr<Json::CharReader> pcr(crb.newCharReader());
    string errs;
    bool ret = pcr->parse(str.c_str(), str.c_str() + str.size(), &root2, &errs);
    //                     起始地址      终止地址          解析后的value  错误原因 

    if (ret == false)
    {
        cout << "json 反序列化失败: " << errs << endl; 
        return;
    }
    
    cout << "----------------------------------------------" << endl;
    cout << "name: " << root2["name"] << endl;
    cout << "age: " << root2["age"] << endl;
    cout << "sex: " << root2["sex"] << endl;
    cout << "score: ";
    for (int i = 0; i < root2["score"].size(); i++)
        cout << root2["score"][i] << " : ";


    cout << endl;
}

int main()
{
    string str;
    serialize(str);
    deserialize(str);

    return 0;
}