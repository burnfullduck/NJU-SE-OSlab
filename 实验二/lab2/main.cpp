#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <string>
using namespace std;
typedef unsigned char uchar;   //1字节
typedef unsigned short ushort; //2字节
typedef unsigned int uint;   //4字节

#pragma pack(1)   //一字节对齐

//创建 RootEntry 结构体类型
struct RootEntry
{
    char DIR_Name[11];//8字节文件名，3字节扩展名
    uchar DIR_Attr;//1字节属性
    char reserve[10];
    ushort DIR_WrtTime;
    ushort DIR_WrtDate;
    ushort DIR_FstClus;//此条目对应的开始簇数
    uint DIR_FileSize;
};

struct Fat12Header
{

    ushort BPB_BytsPerSec; // 每扇区字节数
    uchar BPB_SecPerClus;  // 每簇占用的扇区数
    ushort BPB_RsvdSecCnt; // Boot占用的扇区数
    uchar BPB_NumFATs;     // FAT表的记录数
    ushort BPB_RootEntCnt; // 最大根目录文件数
    ushort BPB_TotSec16;   // 每个FAT占用扇区数
    uchar BPB_Media;       // 媒体描述符
    ushort BPB_FATSz16;    // 每个FAT占用扇区数
    ushort BPB_SecPerTrk;  // 每个磁道扇区数
    ushort BPB_NumHeads;   // 磁头数
    uint BPB_HiddSec;      // 隐藏扇区数
    uint BPB_TotSec32;     // 如果BPB_TotSec16是0，则在这里记录

};


class Node {
private:
    string path;
    string name;
    vector<Node*> children;
    int file_count = 0;
    int dir_count = 0;
    bool isFile = false;
    bool isLayer = false;    //. ..
    uint fileSize;
    char* content = new char[10000];
public:
    Node() = default;
    Node(string name, string path) {
        this->name = name;
        this->path = path;
    }
    Node(string name, bool isLayer) { //create . .. Node
        this->name = name;
        this->isLayer = isLayer;
    }
    void addFileChild(Node* child) {
        this->children.push_back(child);
        this->file_count++;
    }
    void addDirChild(Node* child) {
        this->children.push_back(child);
        child->addLayerChild();
        this->dir_count++;
    }
    void addLayerChild() { //push_back . .. into children
        Node* child1 = new Node(".", true);
        Node* child2 = new Node("..", true);
        this->children.push_back(child1);
        this->children.push_back(child2);
    }
    string getPath() { return this->path; }
    string getName() { return this->name; }
    bool getIsFile() { return this->isFile; }
    bool getIsLayer() { return this->isLayer; }
    int getFileCount() { return this->file_count; }
    int getDirCount() { return this->dir_count; }
    uint getFileSize() { return this->fileSize; }
    char* getContent() { return this->content; }
    void setName(string name) { this->name = name; }
    void setPath(string path) { this->path = path; }
    void setIsFile(bool is) { this->isFile = is; }
    void setSize(uint size) { this->fileSize = size; }
    vector<Node*> getChildren() { return this->children; }
};

int  BytsPerSec;	//每扇区字节数
int  SecPerClus;	//每簇扇区数
int  RsvdSecCnt;	//Boot记录占用的扇区数
int  NumFATs;	//FAT表个数
int  RootEntCnt;	//根目录最大文件数
int  FATSz;	//FAT扇区数
int fatBase;//FAT1起始地址
int fileRootBase;//根目录起始地址
int dataBase;//数据区起始地址
int BytsPerClus;//每簇的字节数

extern "C" {
    void my_print(const char*, const int);
}
void myPrint(const char* s) {
    my_print(s, strlen(s));
}

bool isValidChar(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || (c == ' ');
}

bool isValidName(char* name) {
    for (int i = 0; i < 11; i++) {
        if (!isValidChar(name[i]))
            return false;
    }
    return true;
}

string getFileName(char* name) {
    char result[12];
    int temp = 0;
    for (int i = 0; i < 11; i++) {  //不清楚名称是否有小写 TODO
        if (name[i] == ' ') {
            result[temp] = '.';
            while (name[i] == ' ') {//跳到下一个不为空的字符
                i++;
            }
            i--;  //外循环也会使i+1故要减
        }
        else {
            result[temp] = name[i];
        }
        temp++;
    }
    temp++;
    result[temp] = '\0';
    return result;
}

string getDirName(char* name) {
    char result[12];
    int temp = 0;
    for (int i = 0; i < 11; i++) {
        if (name[i] != ' ')
            result[temp] = name[i];
        else {
            result[temp] = '\0';
            break;
        }
        temp++;
    }
    return result;
}

void fetchContent(FILE* fat12, int startCluster, char* content) {
    if (startCluster == 0 || startCluster == 1)  //0或1无意义,数据区起始与簇2
        return;
    int currentCluster = startCluster;
    int value = 0;
    int pos;

    while (value < 0xFF8) {
        pos = fatBase + currentCluster * 2 / 3; //得到以字节为单位的位置
        ushort bytes;
        ushort* bytes_ptr = &bytes;
        fseek(fat12, pos, SEEK_SET);
        fread(bytes_ptr, 1, 2, fat12);
        if (currentCluster % 2 == 0) {   //!!!为偶去掉高4位,为奇去掉低4位。存储的小尾顺序
            bytes = bytes << 4;
        }
        value = bytes >> 4;

        if (value == 0xFF7) {
            myPrint("读取文件遇到坏簇");
            break;
        }

        char* str = (char*)malloc(BytsPerClus);
        char* ptr = str;
        int startByte = dataBase + (currentCluster - 2) * BytsPerClus;   //数据区从第2簇开始,故减2

        fseek(fat12, startByte, SEEK_SET);
        fread(ptr, 1, BytsPerClus, fat12);

        for (int i = 0; i < BytsPerClus; ++i) {
            *content = ptr[i];
            content++;
        }
        free(str);
        currentCluster = value;
    }
}
void dfs(FILE* fat12, int startCluster, Node* node) {     //广搜处理子目录和子文件
    node->setPath(node->getPath());   //修改路径
    
    if (startCluster == 0 || startCluster == 1)  //0或1无意义,数据区起始与簇2
        return;
    int currentCluster = startCluster;
    int value = 0;
    int pos;

    while (value < 0xFF8) {
        pos = fatBase + currentCluster * 3 / 2; //得到以字节为单位的位置
        ushort bytes;
        ushort* bytes_ptr = &bytes;
        fseek(fat12, pos, SEEK_SET);
        fread(bytes_ptr, 1, 2, fat12);
        if (currentCluster % 2 == 0) {   //!!!为偶去掉高4位,为奇去掉低4位。存储的小尾顺序
            bytes = bytes << 4;
        }
        value = bytes >> 4;
        if (value == 0xFF7) {
            myPrint("读取文件遇到坏簇");
            break;
        }
        int startByte = dataBase + (currentCluster - 2) * BytsPerClus;   //数据区从第2簇开始,故减2

        for (int i = 0; i < BytsPerClus; i = i + 32) {
            RootEntry* rootEntry = new RootEntry();
            fseek(fat12, startByte + i, SEEK_SET);
            fread(rootEntry, 1, 32, fat12);
            if (rootEntry->DIR_Name[0] == '\0' || !isValidName(rootEntry->DIR_Name))  //空或不符合命名规则跳过
                continue;

            if ((rootEntry->DIR_Attr & 0x10) == 0) {//通过DIR_Attr&0x10判断，结果为0是文件，否则为文件夹。
                Node* fchild = new Node();
                fchild->setName(getFileName(rootEntry->DIR_Name));
                fchild->setPath(node->getPath());
                fchild->setIsFile(true);
                fchild->setSize(rootEntry->DIR_FileSize);
                node->addFileChild(fchild);
                fetchContent(fat12, rootEntry->DIR_FstClus, fchild->getContent()); //TODO
            }
            else {
                Node* dchild = new Node();
                dchild->setName(getDirName(rootEntry->DIR_Name));
                dchild->setPath(node->getPath() + getDirName(rootEntry->DIR_Name) + "/");
                node->addDirChild(dchild);
                dfs(fat12, rootEntry->DIR_FstClus, dchild);
            }
        }
    }
}

vector<string> split(const string& str, const string& delim) {      //split代码实现摘自网上
    vector <string> res;
    if ("" == str) return res;

    //先将要切割的字符串从string类型转换为char*类型
    char* strs = new char[str.length() + 1];
    strcpy(strs, str.c_str());
    char* d = new char[delim.length() + 1];
    strcpy(d, delim.c_str());
    char* p = strtok(strs, d);
    while (p) {
        string s = p; //分割得到的字符串转换为string类型
        res.push_back(s); //存入结果数组
        p = strtok(NULL, d);
    }
    return res;
}

void printLS(Node* root) {  //递归
    Node* ptr1 = root;
    Node* ptr2;

    if (ptr1->getIsFile() == true)
        return;
    else {
        string output = ptr1->getPath() + ":" + "\n";
        myPrint(output.c_str());   //输出必须是const 
        output.clear();
        //cout<<ptr1->getChildren().size()<<endl;
        for (int i = 0; i < ptr1->getChildren().size(); i++) {
            ptr2 = ptr1->getChildren()[i];
            if (ptr2->getIsFile())
                output = ptr2->getName() + "  ";
                // cout << ptr2->getName() << " ";
            else
                output = "\033[31m" + ptr2->getName() + "\033[0m" + "  ";
                //cout << "\033[31m" << ptr2->getName() << "\033[0m" << "  ";
                
            myPrint(output.c_str()); 
            output.clear();
        }
        
        myPrint("\n");
        for (int i = 0; i < ptr1->getChildren().size(); i++) {
            if (!ptr1->getChildren()[i]->getIsLayer()) {
                printLS(ptr1->getChildren()[i]);
            }
        }
    }
}

Node* findNode(Node* root, vector<string> dirs) {
    if (dirs.size() == 0)
        return root;
    string name = dirs[0];
    vector<string> newDirs;
    for (int i = 0; i < root->getChildren().size(); i++) {
        if (name == root->getChildren()[i]->getName()) {
            for (int j = 1; j < dirs.size(); i++) {
                newDirs.push_back(dirs[j]);
            }
            return findNode(root->getChildren()[i], newDirs);     //递归寻找Node
        }
    }
    return nullptr;      //找不到返回空指针
}

void printLL(Node* root) {
    //string output;
    Node* ptr1 = root;
    if (ptr1->getIsFile()) {
        myPrint("This path is not a dir, it's a file!");
        return;
    }
    string output = ptr1->getPath() + " " + ptr1->getDirCount() + " " + ptr1->getFileCount() +":"+ "\n";
    //cout << ptr1->getPath() << " " << ptr1->getDirCount() << " " << ptr1->getFileCount() << endl;

    Node* ptr2;
    for (int i = 0; i < ptr1->getChildren().size(); i++) {
        ptr2 = ptr1->getChildren()[i];
        if (ptr2->getIsFile())
            output = ptr2->getName() + " "+":" +ptr2->getFileSize()+"\n";
            //cout << ptr2->getName() << " " << ptr2->getFileSize() << endl;
        else {
            if (ptr2->getIsLayer())
                output = "\033[31m" + ptr2->getName() + "\033[0m" + "  ";
            else
                output = "\033[31m" + ptr2->getName() + "\033[0m" +"  " + ":"+ ptr2->getDirCount() + " " + ptr2->getFileCount()+"\n";
        }
        
    myPrint(output.c_str());
    output.clear();
    }
    myPrint("\n");
    for (int i = 0; i < ptr1->getChildren().size(); i++) {
        if (ptr1->getChildren()[1]->getIsLayer())
            printLL(ptr1->getChildren()[i]);
    }

}

void printCat(Node* root, string path) {
    vector <string> dirs = split(path, "/");
    vector<string> dirs1;
    for(int j=0;j<dirs.size();j++){
        if(dirs[j]=="."){
            continue;
        }else if(dirs[j]==".."){
            if(dirs1.size()>0)
                dirs1.pop_back();
        }else{
            dirs1.push_back(dirs[j]);
        }
    }
    
    Node* ptr = findNode(root, dirs1);
    if (ptr == nullptr) {
        myPrint("Path not found!\n");
    }
    else {
        if (ptr->getIsFile()) {
            myPrint(ptr->getContent());
            myPrint("\n");
        }
        else {
            myPrint("This is a floder!\n");
        }

    }
}

int main() {
    FILE* fat12;
    fat12 = fopen("a.img", "rb");   //读取FAT12镜像文件


    Fat12Header* bpb_ptr = new Fat12Header;
    //初始化
    fseek(fat12, 11, SEEK_SET);   //BPB从偏移11个字节处开始
    fread(bpb_ptr, 1, 25, fat12);   //将引导扇区的数据填入BPB中，
    BytsPerSec = bpb_ptr->BPB_BytsPerSec;
    SecPerClus = bpb_ptr->BPB_SecPerClus;
    RsvdSecCnt = bpb_ptr->BPB_RsvdSecCnt;
    NumFATs = bpb_ptr->BPB_NumFATs;
    RootEntCnt = bpb_ptr->BPB_RootEntCnt;
    if (bpb_ptr->BPB_FATSz16 != 0)
        FATSz = bpb_ptr->BPB_FATSz16;
    else
        FATSz = bpb_ptr->BPB_TotSec32;

    BytsPerClus = SecPerClus * BytsPerSec; //每簇的字节数
    fatBase = RsvdSecCnt * BytsPerSec;     //fat区基址
    fileRootBase = (RsvdSecCnt + NumFATs * FATSz) * BytsPerSec;    //根目录首字节的偏移数=boot+fat1&2的总字节数
    dataBase = fileRootBase + BytsPerSec * ((RootEntCnt * 32 + BytsPerSec - 1) / BytsPerSec);

    //数据区开始扇区号=根目录开始扇区号+根目录所占扇区数。偏移量同理，但注意必须是整扇区
    //如果根目录没有填满最后一个簇，数据区也是从下一个簇开始的，而不是默认于最后一个根目录的结束位置开始。
    //用 （根目录最大文件数 32+每扇区字节数-1）/每扇区字节数* 可以得到根目录区占用的真正扇区数。

    RootEntry* rootEntry_ptr = new RootEntry;

    Node* root = new Node();
    root->setName("");
    root->setPath("/");  //生成根节点

    int frbase = fileRootBase;
    
    for (int i = 0; i < RootEntCnt; i++) {
        fseek(fat12, frbase, SEEK_SET);
        fread(rootEntry_ptr, 1, 32, fat12);
        frbase += 32;
        
        if (rootEntry_ptr->DIR_Name[0] == '\0' || !isValidName(rootEntry_ptr->DIR_Name))  //空或不符合命名规则跳过
            continue;

        if ((rootEntry_ptr->DIR_Attr & 0x10) == 0) {//通过DIR_Attr&0x10判断，结果为0是文件，否则为文件夹。
            
            Node* fchild = new Node();
            fchild->setName(getFileName(rootEntry_ptr->DIR_Name));
            fchild->setPath(root->getPath());
            fchild->setIsFile(true);
            fchild->setSize(rootEntry_ptr->DIR_FileSize);
            root->addFileChild(fchild);
            fetchContent(fat12, rootEntry_ptr->DIR_FstClus, fchild->getContent());
            fchild = nullptr;
            delete fchild;
        }
        else {
            
            Node* dchild = new Node();
            dchild->setName(getDirName(rootEntry_ptr->DIR_Name));
            dchild->setPath(root->getPath() + getDirName(rootEntry_ptr->DIR_Name) + "/");
            root->addDirChild(dchild);
            dfs(fat12, rootEntry_ptr->DIR_FstClus, dchild);
            dchild = nullptr;
            delete dchild;
        }

        
    }

    string wrong = "";
    string input;
    vector<string> cmd;
    bool isL = false;
    bool hasDir = false;
    bool error = false;
    while (true) {
        myPrint(">");
        getline(cin, input);
        cmd = split(input, " ");
        Node* root1 = root;


        if (cmd[0] == "ls") {    //仅ls
            if (cmd.size() == 1) {
                //cout<<root->getDirCount()<<endl; 
                printLS(root);
            }//多个参数判断
            else {
                for (int i = 1; i < cmd.size(); i++) {
                    if (cmd[i][0] == '-') {                     //判断-l
                        for (int j = 1; j < cmd[i].size(); i++) {
                            if (cmd[i][j] == 'l' && !isL) {
                                isL = true;
                            }if (cmd[i][j] != 'l') {
                                error = true;
                                wrong = "Wrong usage!\nusage:\nls \nls -l\nls <path> -l\ncat <filename>\n";
                                break;
                            }
                        }
                        if (error)
                            break;
                    }
                    else if (cmd[i][0] == '/' && !hasDir) {    //判断是否有path并找到
                        vector <string> dirs = split(cmd[i], "/");
                        vector<string> dirs1;
                        for(int j=0;j<dirs.size();j++){
                            if(dirs[j]=="."){
                                continue;
                            }else if(dirs[j]==".."){
                                if(dirs1.size()>0)
                                    dirs1.pop_back();
                            }else{
                                dirs1.push_back(dirs[j]);
                            }
                        }

                        root1 = findNode(root, dirs1);
                        if (root1 == nullptr) {
                            error = true;
                            wrong = "Path not found!\n";
                            break;
                        }
                        else {
                            hasDir = true;
                        }
                    }
                    else {
                        error = true;
                        wrong = "Wrong usage!\nusage:\nls \nls -l\nls <path> -l\ncat <filename>\n";
                        break;
                    }
                }
                if (error) {
                    myPrint(wrong.c_str());
                    continue;
                }if (isL) {
                    if (!hasDir) {    //若没path输出root下
                        printLL(root);
                    }
                    else {
                        printLL(root1);
                    }
                }else if(hasDir){
                    printLS(root1);
                }
            }
        }
        else if (cmd[0] == "cat") {
            if (cmd.size() == 2 && cmd[1][0] != '-') {
                printCat(root, cmd[1]);
            }
            else {
                myPrint("Wrong usage!\nusage:\nls \nls - l\nls <path> -l\ncat <filename>\n");
            }
        }
        else {
            myPrint("Wrong usage!\nusage:\nls \nls - l\nls <path> -l\ncat <filename>\n");
        }
    }
}