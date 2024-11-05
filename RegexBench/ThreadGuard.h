class ThreadGuard
{
public:
    // Move ������
    ThreadGuard(std::thread&& t) : t_(std::move(t)) {}

    // �Ҹ��ڿ��� �����带 join
    ~ThreadGuard()
    {
        if (t_.joinable())
            t_.join();
    }

    // ���� ������ �� ���� �Ҵ� ������ ����
    ThreadGuard(const ThreadGuard&) = delete;
    ThreadGuard& operator=(const ThreadGuard&) = delete;

    // Move �Ҵ� ������
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
