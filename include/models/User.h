#pragma once

#include <memory>
#include <string>

enum class Role {
    Administrator,
    Lecturer,
    HeadOfDepartment,
    Student
};

enum class Permission {
    ManageUsers,
    ManageCourses,
    TeachCourse,
    ModerateDepartment,
    SubmitWork,
    ViewReleasedMarks
};

class User {
public:
    User(int id, std::string username, std::string passwordHash, int departmentId = -1);
    virtual ~User() = default;

    int id() const noexcept;
    const std::string& username() const noexcept;
    const std::string& passwordHash() const noexcept;
    int departmentId() const noexcept;
    virtual Role role() const noexcept = 0;
    virtual bool can(Permission permission) const noexcept = 0;

private:
    int id_;
    std::string username_;
    std::string passwordHash_;
    int departmentId_;
};

class Administrator final : public User {
public:
    using User::User;
    Role role() const noexcept override;
    bool can(Permission permission) const noexcept override;
};

class Lecturer final : public User {
public:
    using User::User;
    Role role() const noexcept override;
    bool can(Permission permission) const noexcept override;
};

class HeadOfDepartment final : public User {
public:
    using User::User;
    Role role() const noexcept override;
    bool can(Permission permission) const noexcept override;
};

class Student final : public User {
public:
    using User::User;
    Role role() const noexcept override;
    bool can(Permission permission) const noexcept override;
};

class UserFactory {
public:
    static std::unique_ptr<User> create(int id, const std::string& username,
                                        const std::string& passwordHash,
                                        const std::string& role, int departmentId);
};

std::string roleToText(Role role);
