class ThreadGuard
{
public:
    // Move 생성자
    ThreadGuard(std::thread&& t) : t_(std::move(t)) {}

    // 소멸자에서 스레드를 join
    ~ThreadGuard()
    {
        if (t_.joinable())
            t_.join();
    }

    // 복사 생성자 및 복사 할당 연산자 삭제
    ThreadGuard(const ThreadGuard&) = delete;
    ThreadGuard& operator=(const ThreadGuard&) = delete;

    // Move 할당 연산자
    ThreadGuard(ThreadGuard&& other) noexcept : t_(std::move(other.t_)) {}
    ThreadGuard& operator=(ThreadGuard&& other) noexcept
    {
        if (this != &other)
        {
            if (t_.joinable())
                t_.join();
            t_ = std::move(other.t_);
        }
        return *this;
    }

private:
    std::thread t_;
};
