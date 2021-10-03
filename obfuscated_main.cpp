#include <cstring>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <iostream>
#include <termios.h>
#include <unistd.h>
#include <list>
#include <sstream>
#include <random>

#define CHECKING_PASSWORD "Please wait. We're checking your password..."
#define OK 0
#define ERROR 1
#define PATH_FATHER_SOCK "/tmp/sock"
#define PATH_CHILD_SOCK "/tmp/sock1"

//Classes with polymorfism to make a fuss in simple strings
class MyString
{
public:
    virtual std::string get_string_fisrt() const { return "Please, enter your password here: "; }
    virtual std::string get_string_second() const { return "Please, wait some time. We are checking your password"; }

};

class MyStringInput: public MyString
{
public:
    std::string get_string_fisrt() const override { return first_sentence; }
    std::string get_string_second() const override { return second_sentence; }
private:
    std::string first_sentence = "Please, enter your new password: ";
    std::string second_sentence = "Please, repeat your new password: ";
};

class MyStringResult : public MyString
{
public:
    std::string get_string_fisrt() const override { return error; }
    std::string get_string_second() const override { return ok; }
private:
    std::string error = "Error. Be more attentive.";
    std::string ok = "Ok. Your password has been changed.";
};


// Enabling and disabling stdin echo in console
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

// Implementation of Elf hash function
std::size_t elf_hash(const std::string& str)
{
    std::size_t hash = 0, x;
    int i = 0;

    for (char c : str)
    {
        // Small algortihm divided by messed up pieces with rubbish code
        while (1)
        {
            step1:
                if (i == 0)
                {
                    ++i;
                    goto step7;
                }
                if (i == 3)
                    goto my_for;
            step2:
                while (i != 2)
                {
                    char e = 'e';
                    // dec code of e is 101. So it is just 1 + 1.
                    i += (static_cast<int>(e) - 100);
                }
                goto step3;
            step3:
                if ((x = (hash & 0xF0000000)))
                    hash ^= x >> 24;
                goto step5;
            step5:
                ++i;
                goto step6;
            step6:
                hash &= ~x;
                goto step1;
            step7:
                hash = (hash << 4) + static_cast<std::size_t>(c);
                std::string process = "PROCESSING_CHECK";
                if (process.size() != 0)
                    goto step2;
        }
        my_for:
            i = 0;
    }
    return hash;
}

std::ostream& operator<<(std::ostream& stream, const std::list<int>& list)
{
    int var = 1;
    auto it = list.begin();
    int i = 0;
    std::size_t val = 0;
    std::size_t len = list.size();

    // Simply iterating and printing list
    while (var != 0)
    {
        switch(var)
        {
        case 1:
            var = 2;
            ++val;
            break;
        case 2:
            // second condition is always true
            if (it != list.end() && (i ^ (i + 1) % 2))
            {
                stream << static_cast<char>(*it);
                var = 3;
            }
            else
            {
                len -= 1;
                var = 0;
            }
            break;
        case 3:
            std::advance(it, 1);
            var = 1;
        }
    }
    return stream;
}

std::list<int> string_to_int_list(const std::string& str)
{
    std::list<int> list_str;
    for (char c : str)
        list_str.emplace_back(static_cast<int>(c));
    return list_str;
}


//for sockets
int create_socket()
{
    int sfd = ::socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sfd == -1)
    {
        std::perror("socket create error");
        throw std::runtime_error("socket create error");
    }
    return sfd;
}

void close_socket(int sfd)
{
    if (::close(sfd) == -1)
    {
        std::perror("socket close error");
        throw std::runtime_error("socket close error");
    }
}

struct Socket
{
    Socket() : sfd(create_socket()) {}

    ~Socket()
    {
        try
        {
            close_socket(sfd);
        }
        catch (const std::exception& e)
        {
            std::cerr << e.what() << std::endl;
        }
    }

    const int sfd;
};

sockaddr_un get_addr_impl(std::string path)
{
    struct sockaddr_un addr;

    ::memset(&addr, 0, sizeof (addr));
    addr.sun_family = AF_UNIX;
    ::strcpy(addr.sun_path, path.c_str());
    return addr;
}

sockaddr_un get_father_addr()
{
    return get_addr_impl(PATH_FATHER_SOCK);
}

sockaddr_un get_child_addr()
{
    return get_addr_impl(PATH_CHILD_SOCK);
}

void bind_wrapper(int sfd, const sockaddr_un& addr)
{
    if (::bind(sfd, reinterpret_cast<const sockaddr*>(&addr), sizeof (addr)) == -1)
    {
        std::perror("bind error");
        throw std::runtime_error("bind error");
    }
}

void sendto_str(int sfd, struct sockaddr_un addr, std::string msg)
{
    socklen_t socklen = sizeof(addr);
    if (::sendto(sfd, msg.data(), msg.size(), 0, reinterpret_cast<sockaddr*>(&addr), socklen) == -1)
    {
        std::perror("send error");
        throw std::runtime_error("send error");
    }
}

std::string recvfrom_str(int sfd, struct sockaddr_un& addr)
{
    socklen_t socklen = sizeof(addr);
    std::string buf(sizeof(std::size_t), '\0');
    if (::recvfrom(sfd, buf.data(), buf.size(), 0, reinterpret_cast<sockaddr*>(&addr), &socklen) == -1)
    {
        std::perror("recvfrom error");
        throw std::runtime_error("recv error");
    }
    return buf;
}

std::size_t get_hashed_password()
{
    std::string psw_to_hash;
    std::size_t hashed_psw;
    ::unlink(PATH_FATHER_SOCK); // without it: bind error: already in use
    ::unlink(PATH_CHILD_SOCK);
    Socket child_sock{};
    Socket father_sock{};

    sockaddr_un father_addr = get_father_addr();
    sockaddr_un child_addr = get_child_addr();
    bind_wrapper(father_sock.sfd, father_addr);
    bind_wrapper(child_sock.sfd, child_addr);

    if (fork() == 0)
    {
        std::string psw;
        set_echo_mode(false);
        std::cin >> psw;
        set_echo_mode(true);
        sendto_str(child_sock.sfd, father_addr, psw);
        exit(1);
    }
    else
    {
        wait(nullptr);
        psw_to_hash = recvfrom_str(father_sock.sfd, child_addr);
        hashed_psw = elf_hash(psw_to_hash);
    }
    return hashed_psw;
}

void print_sentence(const std::string& str)
{
    std::list<int> inp_str = string_to_int_list(str);
    if (inp_str.size() > 0)

        std::cout << inp_str << std::endl;
}

// for absolutely useless function
class StudyPRNG
{
public:
    using result_type = std::uint32_t;
    StudyPRNG(result_type seed)
        :inner_seed(seed){}

    result_type operator()()
    {
        result_type old = inner_seed;
        inner_seed <<= 7;
        inner_seed += old ^ 0x9908b0df;
        return inner_seed;
    }

    result_type min()
    {
        return std::numeric_limits<StudyPRNG::result_type>::min();
    }

    result_type max()
    {
        return std::numeric_limits<StudyPRNG::result_type>::max();
    }

private:
    result_type inner_seed;
};


bool check_password(std::size_t psw1, const std::string& psw = "Enter your password")
{
    print_sentence(CHECKING_PASSWORD);
    StudyPRNG custom_prng(std::random_device{}());

    std::list<int> l = string_to_int_list(psw);
    auto it = l.begin();

    *it *= custom_prng();
    int n = 2;
    switch (n)
    {
    case 0:
        std::advance(it, 1);
        if (*it)
        return OK;
    case 1:
        std::advance(it, 5);
        *it = 10;
        ++n;
        if (*it >= 100)
            return OK;
        else
            return ERROR;
    case 2:
        if (*it == *it)
        return OK;
    }
    return OK;
}

int main()
{
    std::size_t psw1, psw2;
    MyStringInput str_input1;
    MyStringResult str_result1;
    MyString& str_input = str_input1;
    MyString& str_result = str_result1;

    print_sentence(str_input.get_string_fisrt());
    psw1 = get_hashed_password();

    if (check_password(psw1) == ERROR)
        return ERROR;

    print_sentence(str_input.get_string_second());
    psw2 = get_hashed_password();

    if (check_password(psw2) == ERROR)
        return ERROR;

    if (psw1 != psw2)
        print_sentence(str_result.get_string_fisrt());
    else
        print_sentence(str_result.get_string_second());

    return OK;
}

