// ============================================================================
// 计算器程序 - PatchworkOS 图形应用程序
// 功能：提供基本的四则运算计算器界面（数字0-9、+、-、*、/、=、退格）
// ============================================================================

#include <patchwork/patchwork.h>  // PatchworkOS 图形库（窗口、按钮、事件等）
#include <stdint.h>               // 标准整数类型（uint64_t）
#include <stdio.h>                // 标准输入输出
#include <stdlib.h>               // 标准库（内存分配、退出码）
#include <string.h>               // 字符串操作

// ============================================================================
// 界面布局常量定义
// ============================================================================

#define LABEL_ID 1234           // 结果显示标签的唯一标识ID
#define LABEL_HEIGHT (42)       // 结果显示区域的高度（像素）

#define NUMPAD_COLUMNS 4        // 数字键盘列数（4列：7,8,9,/ 等）
#define NUMPAD_ROWS 4           // 数字键盘行数（4行）
#define NUMPAD_PADDING 6        // 按钮之间的间距（像素）
#define NUMPAD_BUTTON_WIDTH (64) // 每个按钮的宽度和高度（正方形按钮）

// 根据列号计算按钮的 X 坐标（像素）
// 公式：左边距 + 按钮宽度 * 列号 + 间距 * (列号+1)
#define NUMPAD_COLUMN_TO_WINDOW(column) \
    (NUMPAD_PADDING * ((column) + 1) + NUMPAD_BUTTON_WIDTH * (column))

// 根据行号计算按钮的 Y 坐标（像素）
// 公式：标签高度 + 上边距 + 按钮高度 * 行号 + 间距 * (行号+2)
#define NUMPAD_ROW_TO_WINDOW(row) \
    (LABEL_HEIGHT + NUMPAD_PADDING * ((row) + 2) + NUMPAD_BUTTON_WIDTH * (row))

// 结果显示标签的宽度（占据整个键盘宽度减去两边间距）
#define LABEL_WIDTH (NUMPAD_COLUMN_TO_WINDOW(NUMPAD_COLUMNS) - NUMPAD_PADDING * 2)

// 整个计算器窗口的宽度和高度
#define WINDOW_WIDTH (NUMPAD_COLUMN_TO_WINDOW(NUMPAD_COLUMNS))
#define WINDOW_HEIGHT (NUMPAD_ROW_TO_WINDOW(NUMPAD_ROWS))

// ============================================================================
// 创建计算器按钮的辅助函数
// ============================================================================

/**
 * 在指定位置创建一个数字键盘按钮
 * @param elem   父元素（窗口对象）
 * @param font   按钮文字使用的字体
 * @param column 列号（0-3，从左到右）
 * @param row    行号（0-3，从上到下）
 * @param name   按钮上显示的文字（如 "1", "+", "=" 等）
 * @param id     按钮的唯一标识符（用于事件识别）
 * @return 成功返回0，失败返回ERR
 */
static uint64_t numpad_button_create(element_t* elem, font_t* font, uint64_t column, uint64_t row, const char* name,
    element_id_t id)
{
    // 根据列号和行号计算按钮的位置和大小
    rect_t rect = RECT_INIT_DIM(NUMPAD_COLUMN_TO_WINDOW(column),  // X坐标
                                 NUMPAD_ROW_TO_WINDOW(row),       // Y坐标
                                 NUMPAD_BUTTON_WIDTH,             // 宽度
                                 NUMPAD_BUTTON_WIDTH);            // 高度
    
    // 创建按钮控件
    element_t* button = button_new(elem, id, &rect, name, ELEMENT_NONE);
    if (button == NULL)
    {
        return ERR;
    }
    
    // 设置按钮的文字字体（统一使用大字体）
    element_get_text_props(button)->font = font;

    return 0;
}

// ============================================================================
// 计算器私有数据结构
// ============================================================================

/**
 * 计算器状态结构体
 * 用于存储计算器的运行时数据
 */
typedef struct
{
    uint64_t input;        // 当前正在输入的数字
    uint64_t accumulator;  // 累积结果（运算中间值）
    char operation;        // 当前要执行的运算符（'+', '-', '*', '/', '='）
    font_t* largeFont;     // 大字体对象（用于显示数字和按钮文字）
} calculator_t;

// ============================================================================
// 窗口事件处理函数（核心逻辑）
// ============================================================================

/**
 * 计算器窗口的事件处理过程
 * @param win   窗口对象
 * @param elem  元素对象（窗口内的根元素）
 * @param event 事件对象
 * @return 成功返回0，失败返回ERR
 */
static uint64_t procedure(window_t* win, element_t* elem, const event_t* event)
{
    switch (event->type)  // 根据事件类型分发处理
    {
    // ------------------------------------------------------------------------
    // 初始化事件：窗口创建时触发，用于创建界面控件
    // ------------------------------------------------------------------------
    case EVENT_LIB_INIT:
    {
        // 分配计算器私有数据
        calculator_t* calc = malloc(sizeof(calculator_t));
        if (calc == NULL)
        {
            return ERR;
        }
        
        // 初始化计算器状态
        calc->input = 0;           // 当前输入为0
        calc->accumulator = 0;     // 累积结果为0
        calc->operation = '=';     // 初始运算符为等号（直接赋值模式）
        
        // 创建大字体（字号32，用于按钮和显示）
        calc->largeFont = font_new(window_get_display(win), "default", "regular", 32);
        if (calc->largeFont == NULL)
        {
            free(calc);
            return ERR;
        }

        // ========== 创建数字按钮 1-9（3x3网格） ==========
        // 布局：
        //   (0,0)=7  (1,0)=8  (2,0)=9
        //   (0,1)=4  (1,1)=5  (2,1)=6
        //   (0,2)=1  (1,2)=2  (2,2)=3
        for (uint64_t column = 0; column < 3; column++)
        {
            for (uint64_t row = 0; row < 3; row++)
            {
                // 计算按钮的数字值：9 - ((2 - column) + row * 3)
                // 这行代码实现了按钮位置的数字映射
                element_id_t id = 9 - ((2 - column) + row * 3);
                char name[2] = {'0' + id, '\0'};  // 数字转字符串

                if (numpad_button_create(elem, calc->largeFont, column, row, name, id) == ERR)
                {
                    font_free(calc->largeFont);
                    free(calc);
                    return ERR;
                }
            }
        }

        // ========== 创建功能按钮 ==========
        // 0 按钮（第1列，第4行）
        if (numpad_button_create(elem, calc->largeFont, 1, 3, "0", 0) == ERR)
        {
            font_free(calc->largeFont);
            free(calc);
            return ERR;
        }
        // 除法 / 按钮（第4列，第1行）
        if (numpad_button_create(elem, calc->largeFont, 3, 0, "/", '/') == ERR)
        {
            font_free(calc->largeFont);
            free(calc);
            return ERR;
        }
        // 乘法 * 按钮（第4列，第2行）
        if (numpad_button_create(elem, calc->largeFont, 3, 1, "*", '*') == ERR)
        {
            font_free(calc->largeFont);
            free(calc);
            return ERR;
        }
        // 减法 - 按钮（第4列，第3行）
        if (numpad_button_create(elem, calc->largeFont, 3, 2, "-", '-') == ERR)
        {
            font_free(calc->largeFont);
            free(calc);
            return ERR;
        }
        // 加法 + 按钮（第4列，第4行）
        if (numpad_button_create(elem, calc->largeFont, 3, 3, "+", '+') == ERR)
        {
            font_free(calc->largeFont);
            free(calc);
            return ERR;
        }
        // 退格 < 按钮（第1列，第4行，但已在0按钮位置？这里有注释需注意）
        if (numpad_button_create(elem, calc->largeFont, 0, 3, "<", '<') == ERR)
        {
            font_free(calc->largeFont);
            free(calc);
            return ERR;
        }
        // 等号 = 按钮（第3列，第4行）
        if (numpad_button_create(elem, calc->largeFont, 2, 3, "=", '=') == ERR)
        {
            font_free(calc->largeFont);
            free(calc);
            return ERR;
        }

        // ========== 创建结果显示标签 ==========
        rect_t labelRect = RECT_INIT_DIM(NUMPAD_PADDING,           // X坐标
                                         NUMPAD_PADDING,           // Y坐标
                                         LABEL_WIDTH,              // 宽度
                                         LABEL_HEIGHT);            // 高度
        
        element_t* label = label_new(elem, LABEL_ID, &labelRect, "0", ELEMENT_NONE);
        if (label == NULL)
        {
            font_free(calc->largeFont);
            free(calc);
            return ERR;
        }

        // 设置标签的文字属性：使用大字体，右对齐
        text_props_t* props = element_get_text_props(label);
        props->font = calc->largeFont;     // 使用大字体
        props->xAlign = ALIGN_MAX;         // 右对齐（MAX = 右对齐）

        // 将计算器私有数据绑定到元素上
        element_set_private(elem, calc);
    }
    break;

    // ------------------------------------------------------------------------
    // 销毁事件：窗口关闭时触发，用于释放资源
    // ------------------------------------------------------------------------
    case EVENT_LIB_DEINIT:
    {
        calculator_t* calc = element_get_private(elem);
        if (calc == NULL)
        {
            break;
        }
        // 释放字体和私有数据
        font_free(calc->largeFont);
        free(calc);
    }
    break;

    // ------------------------------------------------------------------------
    // 按钮动作事件：用户点击按钮时触发
    // ------------------------------------------------------------------------
    case EVENT_LIB_ACTION:
    {
        // 只在按钮释放时处理（按下和释放都触发，我们只响应释放）
        if (event->libAction.type != ACTION_RELEASE)
        {
            break;
        }

        calculator_t* calc = element_get_private(elem);

        // 获取结果显示标签
        element_t* label = element_find(elem, LABEL_ID);
        if (label == NULL)
        {
            return ERR;
        }

        // ========== 处理数字按钮（0-9） ==========
        if (event->libAction.source <= 9)
        {
            // 将新数字追加到当前输入末尾
            // 例如：输入123，再按4 -> 1234
            calc->input = calc->input * 10 + event->libAction.source;
        }
        // ========== 处理退格按钮（<） ==========
        else if (event->libAction.source == '<')
        {
            // 删除最后一位数字（整数除法）
            calc->input /= 10;
        }
        // ========== 处理运算符（+、-、*、/、=） ==========
        else
        {
            // 根据当前存储的运算符执行计算
            switch (calc->operation)
            {
            case '/':  // 除法
                if (calc->input == 0)
                {
                    // 除零错误处理
                    element_set_text(label, "DIV BY ZERO");
                    element_redraw(label, false);
                    return 0;
                }
                calc->accumulator /= calc->input;
                break;
            case '*':  // 乘法
                calc->accumulator *= calc->input;
                break;
            case '-':  // 减法
                calc->accumulator -= calc->input;
                break;
            case '+':  // 加法
                calc->accumulator += calc->input;
                break;
            case '=':  // 等号（初始化或重置）
                calc->accumulator = calc->input;
                break;
            default:
                return 0;
            }
            // 计算完成后，清空当前输入
            calc->input = 0;
            // 保存本次按下的运算符，用于下次计算
            calc->operation = event->libAction.source;
        }

        // ========== 更新显示 ==========
        char buffer[32];
        // 如果按的是等号，显示累积结果；否则显示当前输入
        if (event->libAction.source == '=')
        {
            ulltoa(calc->accumulator, buffer, 10);  // 无符号长长整型转字符串
        }
        else
        {
            ulltoa(calc->input, buffer, 10);
        }
        element_set_text(label, buffer);  // 更新标签文字
        element_redraw(label, false);     // 请求重绘
    }
    break;

    // ------------------------------------------------------------------------
    // 退出事件：窗口关闭时触发
    // ------------------------------------------------------------------------
    case EVENT_LIB_QUIT:
    {
        // 断开显示连接，退出程序
        display_disconnect(window_get_display(win));
    }
    break;
    }

    return 0;
}

// ============================================================================
// 程序入口函数
// ============================================================================

int main(void)
{
    // 创建显示连接（连接到图形服务器）
    display_t* disp = display_new();
    if (disp == NULL)
    {
        return EXIT_FAILURE;
    }

    // 定义窗口位置和大小（位于屏幕坐标 500, 200 处）
    rect_t rect = RECT_INIT_DIM(500, 200, WINDOW_WIDTH, WINDOW_HEIGHT);
    
    // 创建计算器窗口
    // 参数：显示对象、标题、位置大小、窗口类型、装饰、事件处理函数、私有数据
    window_t* win = window_new(disp, "Calculator", &rect, SURFACE_WINDOW, WINDOW_DECO, procedure, NULL);
    if (win == NULL)
    {
        display_free(disp);
        return EXIT_FAILURE;
    }

    // 显示窗口
    if (window_set_visible(win, true) == ERR)
    {
        window_free(win);
        display_free(disp);
        return EXIT_FAILURE;
    }

    // 主事件循环：等待并处理事件
    event_t event = {0};
    while (display_next(disp, &event, CLOCKS_NEVER) != ERR)
    {
        display_dispatch(disp, &event);  // 分发事件到对应的窗口
    }

    // 清理资源
    window_free(win);
    display_free(disp);
    return EXIT_SUCCESS;
}