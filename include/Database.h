#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "models/User.h"

struct UserRecord {
    int id;
    std::string username;
    std::string role;
    int active;
    int departmentId;
};

struct DepartmentRecord {
    int id;
    std::string name;
};

struct CourseRecord {
    int id;
    std::string code;
    std::string title;
    int departmentId;
    std::string departmentName;
};

struct AssignmentRecord {
    int id;
    int courseId;
    std::string courseCode;
    std::string title;
    std::string deadline;
    std::string effectiveDeadline;
    double maxMark;
    std::string attachmentPath;
    int submissionCount;
    int markedCount;
};

struct SubmissionReceipt {
    int id;
    int version;
    std::string status;
    int daysLate;
    double penalty;
    bool markWillBeZero;
    std::string filePath;
};

struct SubmissionRecord {
    int id;
    int assignmentId;
    int studentId;
    int version;
    std::string submittedAt;
    std::string status;
    std::string filePath;
    std::string assignmentTitle;
    std::string courseCode;
    std::string assignmentAttachmentPath;
    std::string studentName;
    double maxMark;
    double penalty;
    double finalMark;
    std::string feedback;
};

struct AuditRecord {
    int id;
    std::string submission;
    std::string changedBy;
    std::optional<double> oldMark;
    double newMark;
    std::string reason;
    std::string changedAt;
};

class Database {
public:
    explicit Database(std::string path = "lms.db");
    ~Database();
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    void initializeSchema();
    void seedDemoAccounts();
    std::unique_ptr<User> authenticate(const std::string& username, const std::string& password);

    int createUser(const User& actor, const std::string& username, const std::string& password,
                   Role role, int departmentId);
    void updateUser(const User& actor, int userId, const std::string& username,
                    Role role, int departmentId, bool active, const std::string& newPassword);
    std::vector<UserRecord> listUsers(const User& actor);

    int createDepartment(const User& actor, const std::string& name);
    void updateDepartment(const User& actor, int departmentId, const std::string& name);
    std::vector<DepartmentRecord> listDepartments(const User& actor);
    int createCourse(const User& actor, const std::string& code, const std::string& title,
                     int departmentId);
    void updateCourse(const User& actor, int courseId, const std::string& code,
                      const std::string& title, int departmentId);
    void assignLecturer(const User& actor, int lecturerId, int courseId);
    void enrolStudent(const User& actor, int studentId, int courseId);
    std::vector<CourseRecord> coursesFor(const User& user);

    std::vector<AssignmentRecord> assignmentsForLecturer(const User& lecturer);
    std::vector<AssignmentRecord> assignmentsForDepartment(const User& hod);
    int createAssignment(const User& actor, int courseId, const std::string& title,
                         const std::string& deadline, double maxMark,
                         const std::string& attachmentPath = "");
    void grantExtension(const User& actor, int assignmentId, int studentId,
                        const std::string& newDeadline, const std::string& reason);
    std::vector<AssignmentRecord> assignmentsForStudent(const User& student);
    SubmissionReceipt submit(const User& student, int assignmentId, const std::string& filePath);
    std::vector<SubmissionRecord> submissionsForLecturer(const User& lecturer);
    void saveMark(const User& lecturer, int submissionId, double rawMark,
                  const std::string& feedback, const std::string& reason);
    std::vector<SubmissionRecord> submissionsForModeration(const User& hod);
    std::vector<SubmissionRecord> submissionsReadyForRelease(const User& hod);
    void moderateSubmission(const User& hod, int submissionId,
                            const std::optional<double>& revisedMark,
                            const std::string& reason);
    void releaseModerated(const User& hod, int submissionId);
    std::vector<SubmissionRecord> releasedMarks(const User& student);
    std::vector<AuditRecord> auditForDepartment(const User& hod);

private:
    std::string dbPath_;
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
