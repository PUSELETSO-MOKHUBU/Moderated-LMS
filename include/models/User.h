#pragma once

#include <string>

enum class Role {
    Administrator,
    Lecturer,
    HeadOfDepartment,
    Student
};

class User {
public:
    User();
    User(int id, std::string username, std::string passwordHash, Role role, int departmentId = -1);

    int id() const;
    const std::string& username() const;
    const std::string& passwordHash() const;
    Role role() const;
    int departmentId() const;

    void setId(int id);
    void setUsername(const std::string& username);
    void setPasswordHash(const std::string& passwordHash);
    void setRole(Role role);
    void setDepartmentId(int departmentId);

private:
    int id_;
    std::string username_;
    std::string passwordHash_;
    Role role_;
    int departmentId_;
};
