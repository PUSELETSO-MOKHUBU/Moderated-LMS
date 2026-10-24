#pragma once

#include <string>

enum class SubmissionStatus {
    Submitted,
    Late,
    Marked,
    Moderated,
    Released
};

class Submission {
public:
    Submission();
    Submission(int id, int assignmentId, int studentId, int versionNo, std::string submittedAt,
               SubmissionStatus status, std::string filePath, bool isActive);

    int id() const;
    int assignmentId() const;
    int studentId() const;
    int versionNo() const;
    const std::string& submittedAt() const;
    SubmissionStatus status() const;
    const std::string& filePath() const;
    bool isActive() const;

    void setId(int id);
    void setAssignmentId(int assignmentId);
    void setStudentId(int studentId);
    void setVersionNo(int versionNo);
    void setSubmittedAt(const std::string& submittedAt);
    void setStatus(SubmissionStatus status);
    void setFilePath(const std::string& filePath);
    void setIsActive(bool isActive);

private:
    int id_;
    int assignmentId_;
    int studentId_;
    int versionNo_;
    std::string submittedAt_;
    SubmissionStatus status_;
    std::string filePath_;
    bool isActive_;
};
