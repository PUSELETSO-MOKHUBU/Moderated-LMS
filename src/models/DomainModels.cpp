#include "models/User.h"
#include "models/Course.h"
#include "models/Assignment.h"
#include "models/Submission.h"
#include "models/MarkAudit.h"

User::User() : id_(-1), username_(), passwordHash_(), role_(Role::Student), departmentId_(-1) {}

User::User(int id, std::string username, std::string passwordHash, Role role, int departmentId)
    : id_(id), username_(std::move(username)), passwordHash_(std::move(passwordHash)), role_(role), departmentId_(departmentId) {}

int User::id() const { return id_; }
const std::string& User::username() const { return username_; }
const std::string& User::passwordHash() const { return passwordHash_; }
Role User::role() const { return role_; }
int User::departmentId() const { return departmentId_; }
void User::setId(int id) { id_ = id; }
void User::setUsername(const std::string& username) { username_ = username; }
void User::setPasswordHash(const std::string& passwordHash) { passwordHash_ = passwordHash; }
void User::setRole(Role role) { role_ = role; }
void User::setDepartmentId(int departmentId) { departmentId_ = departmentId; }

Course::Course() : id_(-1), code_(), title_(), departmentId_(-1) {}
Course::Course(int id, std::string code, std::string title, int departmentId)
    : id_(id), code_(std::move(code)), title_(std::move(title)), departmentId_(departmentId) {}
int Course::id() const { return id_; }
const std::string& Course::code() const { return code_; }
const std::string& Course::title() const { return title_; }
int Course::departmentId() const { return departmentId_; }
void Course::setId(int id) { id_ = id; }
void Course::setCode(const std::string& code) { code_ = code; }
void Course::setTitle(const std::string& title) { title_ = title; }
void Course::setDepartmentId(int departmentId) { departmentId_ = departmentId; }

Assignment::Assignment() : id_(-1), courseId_(-1), title_(), deadline_(), maxMark_(0.0) {}
Assignment::Assignment(int id, int courseId, std::string title, std::string deadline, double maxMark)
    : id_(id), courseId_(courseId), title_(std::move(title)), deadline_(std::move(deadline)), maxMark_(maxMark) {}
int Assignment::id() const { return id_; }
int Assignment::courseId() const { return courseId_; }
const std::string& Assignment::title() const { return title_; }
const std::string& Assignment::deadline() const { return deadline_; }
double Assignment::maxMark() const { return maxMark_; }
void Assignment::setId(int id) { id_ = id; }
void Assignment::setCourseId(int courseId) { courseId_ = courseId; }
void Assignment::setTitle(const std::string& title) { title_ = title; }
void Assignment::setDeadline(const std::string& deadline) { deadline_ = deadline; }
void Assignment::setMaxMark(double maxMark) { maxMark_ = maxMark; }

Submission::Submission() : id_(-1), assignmentId_(-1), studentId_(-1), versionNo_(1), submittedAt_(), status_(SubmissionStatus::Submitted), filePath_(), isActive_(true) {}
Submission::Submission(int id, int assignmentId, int studentId, int versionNo, std::string submittedAt,
                     SubmissionStatus status, std::string filePath, bool isActive)
    : id_(id), assignmentId_(assignmentId), studentId_(studentId), versionNo_(versionNo),
      submittedAt_(std::move(submittedAt)), status_(status), filePath_(std::move(filePath)), isActive_(isActive) {}
int Submission::id() const { return id_; }
int Submission::assignmentId() const { return assignmentId_; }
int Submission::studentId() const { return studentId_; }
int Submission::versionNo() const { return versionNo_; }
const std::string& Submission::submittedAt() const { return submittedAt_; }
SubmissionStatus Submission::status() const { return status_; }
const std::string& Submission::filePath() const { return filePath_; }
bool Submission::isActive() const { return isActive_; }
void Submission::setId(int id) { id_ = id; }
void Submission::setAssignmentId(int assignmentId) { assignmentId_ = assignmentId; }
void Submission::setStudentId(int studentId) { studentId_ = studentId; }
void Submission::setVersionNo(int versionNo) { versionNo_ = versionNo; }
void Submission::setSubmittedAt(const std::string& submittedAt) { submittedAt_ = submittedAt; }
void Submission::setStatus(SubmissionStatus status) { status_ = status; }
void Submission::setFilePath(const std::string& filePath) { filePath_ = filePath; }
void Submission::setIsActive(bool isActive) { isActive_ = isActive; }

MarkAudit::MarkAudit() : id_(-1), submissionId_(-1), changedBy_(-1), oldMark_(0.0), newMark_(0.0), reason_(), changedAt_() {}
MarkAudit::MarkAudit(int id, int submissionId, int changedBy, double oldMark, double newMark,
                   const std::string& reason, const std::string& changedAt)
    : id_(id), submissionId_(submissionId), changedBy_(changedBy), oldMark_(oldMark), newMark_(newMark),
      reason_(reason), changedAt_(changedAt) {}
int MarkAudit::id() const { return id_; }
int MarkAudit::submissionId() const { return submissionId_; }
int MarkAudit::changedBy() const { return changedBy_; }
double MarkAudit::oldMark() const { return oldMark_; }
double MarkAudit::newMark() const { return newMark_; }
const std::string& MarkAudit::reason() const { return reason_; }
const std::string& MarkAudit::changedAt() const { return changedAt_; }
void MarkAudit::setId(int id) { id_ = id; }
void MarkAudit::setSubmissionId(int submissionId) { submissionId_ = submissionId; }
void MarkAudit::setChangedBy(int changedBy) { changedBy_ = changedBy; }
void MarkAudit::setOldMark(double oldMark) { oldMark_ = oldMark; }
void MarkAudit::setNewMark(double newMark) { newMark_ = newMark; }
void MarkAudit::setReason(const std::string& reason) { reason_ = reason; }
void MarkAudit::setChangedAt(const std::string& changedAt) { changedAt_ = changedAt; }
