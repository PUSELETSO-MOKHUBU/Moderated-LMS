#pragma once

#include <string>

class MarkAudit {
public:
    MarkAudit();
    MarkAudit(int id, int submissionId, int changedBy, double oldMark, double newMark,
             const std::string& reason, const std::string& changedAt);

    int id() const;
    int submissionId() const;
    int changedBy() const;
    double oldMark() const;
    double newMark() const;
    const std::string& reason() const;
    const std::string& changedAt() const;

    void setId(int id);
    void setSubmissionId(int submissionId);
    void setChangedBy(int changedBy);
    void setOldMark(double oldMark);
    void setNewMark(double newMark);
    void setReason(const std::string& reason);
    void setChangedAt(const std::string& changedAt);

private:
    int id_;
    int submissionId_;
    int changedBy_;
    double oldMark_;
    double newMark_;
    std::string reason_;
    std::string changedAt_;
};
