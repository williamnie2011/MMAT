#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/rand.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"

const int TOTAL = 5;
const int MAXQ  = 32;
const int LCD_COLS = 16;

const char* SYM[] = {"+", "-", "x", "/"};

//LCD1402
#define I2C_PORT i2c0
#define PIN_SDA  4
#define PIN_SCL  5
#define LCD_ADDR 0x27
#define LCD_BL   0x08
#define LCD_EN   0x04
#define LCD_RS   0x01

//键盘
const uint8_t ROWS[4] = {6, 7, 8, 9};
const uint8_t COLS[4] = {10, 11, 12, 13};
const char KEYMAP[4][4] = {
    {'1','2','3','C'},
    {'4','5','6','E'},
    {'7','8','9','.'},
    {'-','0','#','*'}
};

//蜂鸣器
#define PIN_BUZZER 15

void lcd_send(uint8_t nib, bool rs){
    uint8_t base = (uint8_t)((nib << 4) | LCD_BL | (rs ? LCD_RS : 0));
    uint8_t hi = base | LCD_EN;
    uint8_t lo = base & (uint8_t)~LCD_EN;
    i2c_write_blocking(I2C_PORT, LCD_ADDR, &hi, 1, false);
    sleep_us(1);
    i2c_write_blocking(I2C_PORT, LCD_ADDR, &lo, 1, false);
    sleep_us(60);
}

void lcd_cmd(uint8_t c){
    lcd_send(c >> 4, false);
    lcd_send(c & 0x0F, false);
}

void lcd_data(uint8_t d){
    lcd_send(d >> 4, true);
    lcd_send(d & 0x0F, true);
}

void lcd_init(){
    sleep_ms(60);
    lcd_send(0x03, false); sleep_ms(5);
    lcd_send(0x03, false); sleep_us(150);
    lcd_send(0x03, false);
    lcd_send(0x02, false);
    lcd_cmd(0x28);
    lcd_cmd(0x0C);
    lcd_cmd(0x06);
    lcd_cmd(0x01);
    sleep_ms(3);
}

void lcd_goto(int col, int row){
    lcd_cmd((uint8_t)(0x80 | (row ? 0x40 : 0x00) | col));
}

void lcd_print(const char* s){
    while(*s) lcd_data((uint8_t)*s++);
}

void lcd_blank(int row){
    lcd_goto(0, row);
    for(int i = 0; i < LCD_COLS; i++) lcd_data(' ');
    lcd_goto(0, row);
}

void buzzer_init(){
    gpio_set_function(PIN_BUZZER, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(PIN_BUZZER);
    pwm_config cfg = pwm_get_default_config();
    pwm_config_set_clkdiv(&cfg, 125.0f);
    pwm_config_set_wrap(&cfg, 1000);
    pwm_init(slice, &cfg, true);
    pwm_set_gpio_level(PIN_BUZZER, 0);
}

void beep(uint wrap, uint ms){
    uint slice = pwm_gpio_to_slice_num(PIN_BUZZER);
    pwm_set_wrap(slice, wrap);
    pwm_set_gpio_level(PIN_BUZZER, wrap / 2);
    sleep_ms(ms);
    pwm_set_gpio_level(PIN_BUZZER, 0);
}

char keypad_scan(){
    for(int r = 0; r < 4; r++){
        gpio_put(ROWS[r], 0);
        for(int c = 0; c < 4; c++){
            if(gpio_get(COLS[c]) == 0){
                sleep_ms(20);
                if(gpio_get(COLS[c]) == 0){
                    while(gpio_get(COLS[c]) == 0) tight_loop_contents();
                    gpio_put(ROWS[r], 1);
                    return KEYMAP[r][c];
                }
            }
        }
        gpio_put(ROWS[r], 1);
    }
    return 0;
}

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
        a = (int)(get_rand_32() % 9) + 1;
        b = (int)(get_rand_32() % 9) + 1;
        if(cnt++ > 10000){
            a = 1;
            b = 1;
            break;
        }
    }while(!legal(k,a,b));
}

int ask(int k,int a,int b){
    char buf[8] = "";
    int len = 0;
    char line[24];

    snprintf(line, sizeof(line), "%d %s %d =", a, SYM[k], b);
    lcd_goto(0, 0);
    lcd_print(line);
    lcd_blank(1);

    while(true){
        char key = keypad_scan();
        if(key == 0) continue;
        if(key == 'C'){
            buf[0] = 0;
            len = 0;
            lcd_blank(1);
            beep(600, 30);
        }else if(key == 'E'){
            break;
        }else if(key >= '0' && key <= '9'){
            if(len < 3){
                buf[len++] = key;
                buf[len] = 0;
                lcd_data((uint8_t)key);
                beep(1200, 20);
            }
        }
    }
    return atoi(buf);
}

void show_result(bool ok,int answer,int score,int streak){
    char line[24];
    if(ok){
        snprintf(line, sizeof(line), "OK  S:%d  R:%d", score, streak);
        beep(1400, 120);
    }else{
        snprintf(line, sizeof(line), "NO  =%d", answer);
        beep(400, 250);
    }
    lcd_blank(1);
    lcd_print(line);
}

int main(){
    stdio_init_all();

    i2c_init(I2C_PORT, 100 * 1000);
    gpio_set_function(PIN_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PIN_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(PIN_SDA);
    gpio_pull_up(PIN_SCL);

    for(int i = 0; i < 4; i++){
        gpio_init(ROWS[i]);
        gpio_set_dir(ROWS[i], GPIO_OUT);
        gpio_put(ROWS[i], 1);
        gpio_init(COLS[i]);
        gpio_set_dir(COLS[i], GPIO_IN);
        gpio_pull_up(COLS[i]);
    }

    buzzer_init();
    lcd_init();

    srand(get_rand_32());

    int score = 0, streak = 0, beststreak = 0;
    int wrong_a[MAXQ], wrong_b[MAXQ], wrong_k[MAXQ];
    int wrong_ans = 0, wrong_cnt = 0;

    for(int i = 0; i < MAXQ; i++){
        wrong_a[i] = -1;
        wrong_b[i] = -1;
        wrong_k[i] = -1;
    }

    for(int i = 0; i < TOTAL; i++){
        int k = (int)(get_rand_32() % 4);
        int a, b;
        gen(k, a, b);

        int user = ask(k, a, b);
        int right = question(k, a, b);

        if(user == right){
            score++;
            streak++;
            if(streak > beststreak) beststreak = streak;
            show_result(true, right, score, streak);
        }else{
            if(score > 0) score--;
            streak = 0;
            wrong_a[wrong_cnt] = a;
            wrong_b[wrong_cnt] = b;
            wrong_k[wrong_cnt] = k;
            wrong_cnt++;
            wrong_ans++;
            show_result(false, right, score, streak);
        }
        sleep_ms(900);
    }

    lcd_goto(0, 0);
    lcd_print("错题重做");
    sleep_ms(600);

    while(wrong_ans > 0){
        for(int i = 0; i < wrong_cnt; i++){
            if(wrong_a[i] == -1) continue;
            int user = ask(wrong_k[i], wrong_a[i], wrong_b[i]);
            int right = question(wrong_k[i], wrong_a[i], wrong_b[i]);
            if(user == right){
                wrong_ans--;
                wrong_a[i] = -1;
                show_result(true, right, score, streak);
            }else{
                show_result(false, right, score, streak);
            }
            sleep_ms(700);
        }
    }

    char line[24];
    lcd_blank(0);
    lcd_blank(1);
    lcd_goto(0, 0);
    lcd_print("Done!");
    snprintf(line, sizeof(line), "S:%d B:%d", score, beststreak);
    lcd_goto(0, 1);
    lcd_print(line);
    beep(1400, 150);
    sleep_ms(150);
    beep(1400, 250);

    while(true) tight_loop_contents();
}
