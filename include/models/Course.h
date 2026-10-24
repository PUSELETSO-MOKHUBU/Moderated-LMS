#pragma once

#include <string>

class Course {
public:
    Course();
    Course(int id, std::string code, std::string title, int departmentId);

    int id() const;
    const std::string& code() const;
    const std::string& title() const;
    int departmentId() const;

    void setId(int id);
    void setCode(const std::string& code);
    void setTitle(const std::string& title);
    void setDepartmentId(int departmentId);

private:
    int id_;
    std::string code_;
    std::string title_;
    int departmentId_;
};
