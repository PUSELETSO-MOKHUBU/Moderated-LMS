#pragma once

#include <string>

class Assignment {
public:
    Assignment();
    Assignment(int id, int courseId, std::string title, std::string deadline, double maxMark);

    int id() const;
    int courseId() const;
    const std::string& title() const;
    const std::string& deadline() const;
    double maxMark() const;

    void setId(int id);
    void setCourseId(int courseId);
    void setTitle(const std::string& title);
    void setDeadline(const std::string& deadline);
    void setMaxMark(double maxMark);

private:
    int id_;
    int courseId_;
    std::string title_;
    std::string deadline_;
    double maxMark_;
};
