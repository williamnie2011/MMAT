#include <iostream>
#include <string>
#include <ctime>
#include <cstdlib>
#include <cmath>
using namespace std;

const int TOTAL = 5;
const int MAXQ  = 32;

const char* SYM[] = {"+", "-", "x", "/"};

//答案生成
int add(int a,int b){ 
    return a+b;
}
int sub(int a,int b){ 
    return a-b;
}
int mul(int a,int b){
    return a*b;
}
int divide(int a,int b){
    return a/b;
}

//随机出题
int question(int k, int a, int b){
    switch(k){
        case 0: return add(a, b);
        case 1: return sub(a, b);
        case 2: return mul(a, b);
        case 3: return divide(a, b);
    }
    return 0;
}

bool legal(int k,int a,int b){
    switch(k){
        case 0: return a + b <= 10; 
        case 1: return a - b >= 0;
        case 2: return a * b <= 100;
        case 3: return b != 0 && a % b == 0;
    }
    return false;
}

void gen(int k,int &a,int &b){
    int cnt = 0;
    do{
        a = rand() % 9 + 1;
        b = rand() % 9 + 1;
        if(cnt++ > 10000){
            a = 1;
            b = 1;
            break;
        }
    }while(!legal(k,a,b));
}

int main(){
    srand((unsigned)time(0));

    int score=0, streak=0, beststreak=0;

    int wrong_a[MAXQ]={0}, wrong_b[MAXQ]={0}, wrong_k[MAXQ]={0};
    int wrong_ans=0, wrong_cnt=0;
    string ans;

    for(int i=0;i<MAXQ;i++){
        wrong_a[i] = -1;
        wrong_b[i] = -1;
        wrong_k[i] = -1;
    }

    for(int i=0;i<TOTAL;i++){
        int k = rand() % 4;
        int a,b;
        gen(k,a,b);

        cout << a << " " << SYM[k] << " " << b << " = ";
        if(!(cin >> ans)) return 0;

        if(ans == to_string(question(k, a, b))){
            cout << "Accepted!" << endl;
            score++;
            streak++;
            beststreak = max(beststreak, streak);
        }else{
            cout << "Wrong Answer!" << endl;
            if(score > 0) score--;
            streak = 0;
            wrong_a[wrong_cnt] = a;
            wrong_b[wrong_cnt] = b;
            wrong_k[wrong_cnt] = k;
            wrong_cnt++;
            wrong_ans++;
        }
        cout << "  Score: " << score << "   Streak: " << streak << endl;
    }

    cout << "\n错题数: " << wrong_ans << endl;
    while(wrong_ans > 0){
        for(int i=0;i<wrong_cnt;i++){
            if(wrong_a[i] == -1) continue;
            cout << "[重做] " << wrong_a[i] << " " << SYM[wrong_k[i]] << " " << wrong_b[i] << " = ";
            if(!(cin >> ans)) return 0;
            if(ans == to_string(question(wrong_k[i],wrong_a[i],wrong_b[i]))){
                cout << "Correct!" << endl;
                wrong_ans--;
                wrong_a[i] = -1;
            }else{
                cout << "Wrong Answer! " << question(wrong_k[i],wrong_a[i],wrong_b[i]) << endl;
            }
        }
    }

    cout << "Score:     " << score << endl;
    cout << "BestStreak: " << beststreak << endl;
    cout << "Done!" << endl;
    return 0;
}
