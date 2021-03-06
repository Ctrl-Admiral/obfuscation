# Защита программ и данных
## Курс: https://www.hse.ru/edu/courses/339492221

### Задание 1. Обфускация программ.

Алгоритм работы программы:

1. Запрашивает у пользователя введение пароля.
2. Для скрытного ввода пароля отключает режим `echo` для потока ввода, получает от пользователя строку, хэширует ее и сохраняет.
3. Повторяются шаги 1-2.
4. Сравниваются два полученных хэша. Если они равны, выводится сообщение об успехе изменения пароля: `"Ok. Your password has been changed."`
В случае неудачи ввыодится сообщение об ошибке: `"Error. Be more attentive."`.

Код программы до обфускации:

```
#include <iostream>
#include <termios.h>
#include <unistd.h>

void set_echo_mode(bool enable)
{
    struct termios tty;
    tcgetattr(STDIN_FILENO, &tty);
    if (enable)
        tty.c_lflag |= ECHO;
    else
        tty.c_lflag &= ~ECHO;

    (void) tcsetattr(STDIN_FILENO, TCSANOW, &tty);
}

std::size_t elf_hash(const std::string& str)
{
    std::size_t hash = 0, x;

    for (char c : str)
    {
        hash = (hash << 4) + static_cast<std::size_t>(c);
        if ((x = (hash & 0xF0000000)))
            hash ^= x >> 24;
        hash &= ~x;
    }
    return hash;
}

int main()
{
    std::string psw1, psw2;
    set_echo_mode(false);
    std::cout << "Please, input new password: ";
    std::cin >> psw1;
    std::size_t psw1_hash = elf_hash(psw1);
    std::cout << "\nPlease, repeat your new password: ";
    std::cin >> psw2;
    std::size_t psw2_hash = elf_hash(psw2);
    set_echo_mode(true);
    if (psw1_hash == psw2_hash)
        std::cout << "\nOK. Password has been changed.\n";
    else
        std::cout << "\nError. Be more attentive.\n";
    return 0;
}
```

---
Код с применением методов обфускации находится в файле `obfuscated_main.cpp`.

Для усложнения антиотладки кода использовались следующие средства.

Обфускация данных:
- Конвертация `std::string` в `std::list<int>` -- списка десятичных ASCII-кодов символов. Это двусвязный список, и элементы могут быть разбросаны по памяти.
- Для хранения самих строк создана система наследуемых классов с полиморфизмом. Класс-родитель не имеет никакого смысла, содержит в себе две виртуальные функции. Классы-дети переопределяют функции для использования в качестве getter'ов и содержат в себе по две строки.

Обфускация управления:
- В перегрузке оператора `<<` используется ненужный switch-case, маскирующий обычный цикл прохода по элементам массива.
- В функции хэширования `elf_hash()` алгоритм разбит на части и спутан с помощью многочисленных `goto` конструкций. Кроме того, в тело цикла `for` для итерации по списку добавлен ненужный внутренний цикл `while(1)`, тело которого на самом деле будет исполняться 1 раз для каждой итерации внешнего цикла `for`.
- В функции получения от пользователя пароля `get_password()` с помощью `fork()` запускается дочерний процесс, получающий от пользователя пароль и передающий его с помощью сокета родительскому процессу для хэширования.
- Функция `check_password()` совершенно бесполезна, и всегда будет возвращать код `OK`. Нужна только для усложнения анализа кода.

В скрипте конфигурации и сборки:
- Утилита `strip` для удаления всех символов в объектном файле
- Утилита `upx` -- упаковщик исполняемых файлов
- Флаг `-funroll-loops` для отмены сворачивания структуры циклов.
