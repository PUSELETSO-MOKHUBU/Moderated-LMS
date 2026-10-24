#include "Database.h"
#include "FileStorage.h"
#include "LatePenaltyStrategy.h"
#include "PasswordHasher.h"

#include <cstdio>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
void expect(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

template <typename Function>
void expectFailure(Function action, const std::string& message) {
    bool failed = false;
    try {
        action();
    } catch (const std::exception&) {
        failed = true;
    }
    expect(failed, message);
}
}

int main() {
    const std::filesystem::path testRoot = std::filesystem::temp_directory_path() /
        ("moderated_lms_test_" + std::to_string(
            std::chrono::high_resolution_clock::now().time_since_epoch().count()));
    const std::string path = (testRoot / "lms_test.sqlite3").string();
    const std::filesystem::path uploadRoot = testRoot / "uploads";
    const std::filesystem::path sourceDocument = testRoot / "lms_test_brief.PDF";
    std::filesystem::create_directories(testRoot);
    try {
        const std::string hash = PasswordHasher::hash("correct horse");
        expect(hash != "correct horse", "Passwords must not be stored as plaintext.");
        expect(PasswordHasher::verify("correct horse", hash), "Password hash verification failed.");
        expect(!PasswordHasher::verify("wrong", hash), "Incorrect password was accepted.");
        expect(PasswordHasher::hash("correct horse") != hash, "Password salt must be random.");

        StandardLatePenalty penalty;
        expect(penalty.penalty(100.0, 0) == 0.0, "On-time work should have no penalty.");
        expect(penalty.penalty(100.0, 1) == 10.0, "One-day penalty must be 10%.");
        expect(penalty.penalty(100.0, 3) == 30.0, "Three-day penalty must be 30%.");
        expect(penalty.penalty(100.0, 5) == 30.0, "The standard penalty is capped at three days.");
        expect(penalty.finalMark(80.0, 100.0, 1) == 70.0, "Final mark must subtract the late penalty.");
        expect(penalty.finalMark(80.0, 100.0, 4) == 0.0, "Work over three days late must receive zero.");

        {
            std::ofstream source(sourceDocument, std::ios::binary);
            source << "%PDF-1.4 test document";
        }
        FileStorage fileStorage(uploadRoot);
        const std::string storedBrief = fileStorage.storeDocument(sourceDocument.string(), UploadKind::Assignment);
        expect(std::filesystem::exists(storedBrief) &&
               std::filesystem::path(storedBrief).extension() == ".pdf",
               "Assignment uploads must be copied into controlled storage and preserve their format.");
        expect(FileStorage::isSupportedDocument("brief.DOCX"),
               "Common Word document formats must be accepted case-insensitively.");
        const std::filesystem::path unsupportedDocument = testRoot / "program.exe";
        {
            std::ofstream unsupported(unsupportedDocument, std::ios::binary);
            unsupported << "not a document";
        }
        expectFailure([&] {
            fileStorage.storeDocument(unsupportedDocument.string(), UploadKind::Submission);
        }, "Unsupported document files must be rejected.");
        expectFailure([&] {
            fileStorage.storeDocument((testRoot / "missing.pdf").string(), UploadKind::Submission);
        }, "Missing document files must be rejected.");

        {
            Database database(path);
            database.initializeSchema();
            database.seedDemoAccounts();
            auto admin = database.authenticate("admin", "admin123");
            auto lecturer = database.authenticate("lecturer", "lecturer123");
            auto hod = database.authenticate("hod", "hod123");
            auto student = database.authenticate("student", "student123");
            expect(admin && lecturer && hod && student, "Each of the four seeded role accounts must authenticate.");
            expect(database.authenticate("lecture", "lecturer123") != nullptr,
                   "Legacy lecturer login alias must authenticate.");
            expect(database.authenticate("head", "hod123") != nullptr,
                   "Legacy HoD login alias must authenticate.");
            expect(PasswordHasher::isEncodedHash(admin->passwordHash()),
                   "Database authentication must return an encoded password hash, not plaintext.");
            expect(!database.authenticate("student", "incorrect"), "Invalid credentials must be rejected.");
            expect(admin->role() == Role::Administrator && lecturer->role() == Role::Lecturer &&
                   hod->role() == Role::HeadOfDepartment && student->role() == Role::Student,
                   "UserFactory must instantiate the requested role.");
            expectFailure([&] {
                database.createAssignment(*student, 1, "Forbidden", "2099-01-01 12:00", 100.0);
            }, "Students must not create assignments.");

            const auto lecturerCourses = database.coursesFor(*lecturer);
            expect(lecturerCourses.size() == 1, "Lecturer should only see assigned courses.");
            const auto hodCourses = database.coursesFor(*hod);
            expect(hodCourses.size() == 1 && hodCourses.front().id == lecturerCourses.front().id,
                   "The demo HoD must be assigned to the General department and see its courses.");
            const int courseId = lecturerCourses.front().id;
            const int assignmentId = database.createAssignment(*lecturer, courseId, "Integration Test",
                                                                 "2099-12-31 23:59", 100.0, storedBrief);
            const auto departmentAssignments = database.assignmentsForDepartment(*hod);
            expect(departmentAssignments.size() == 1 &&
                   departmentAssignments.front().attachmentPath == storedBrief,
                   "HoDs must see department assignments and their uploaded briefs.");
            const int extensionAssignment = database.createAssignment(*lecturer, courseId, "Extension Test",
                                                                        "2099-11-30 23:59", 100.0);
            database.grantExtension(*lecturer, extensionAssignment, student->id(),
                                    "2099-12-15 23:59", "Approved extension");
            bool extensionFound = false;
            for (const auto& assignment : database.assignmentsForStudent(*student)) {
                if (assignment.id == extensionAssignment) {
                    extensionFound = assignment.effectiveDeadline == "2099-12-15T23:59:00";
                }
            }
            expect(extensionFound, "Individual extension must replace the student's effective deadline.");

            const std::string firstDocument = fileStorage.storeDocument(sourceDocument.string(), UploadKind::Submission);
            const std::string secondDocument = fileStorage.storeDocument(sourceDocument.string(), UploadKind::Submission);
            const SubmissionReceipt first = database.submit(*student, assignmentId, firstDocument);
            const SubmissionReceipt second = database.submit(*student, assignmentId, secondDocument);
            const int firstVersion = first.id;
            const int secondVersion = second.id;
            expect(firstVersion != secondVersion, "Resubmission must create a new persistent version.");
            expect(second.filePath == secondDocument,
                   "The submission receipt must identify the stored versioned document.");
            auto submissions = database.submissionsForLecturer(*lecturer);
            expect(submissions.size() == 1 && submissions.front().version == 2 &&
                   submissions.front().id == secondVersion && submissions.front().filePath == secondDocument &&
                   submissions.front().assignmentAttachmentPath == storedBrief,
                   "The newest resubmission must be the active version.");

            database.saveMark(*lecturer, secondVersion, 80.0, "Good work", "ignored initial reason");
            expect(database.releasedMarks(*student).empty(), "Lecturer-marked results must remain hidden.");
            const auto departmentAssignment = database.assignmentsForDepartment(*hod);
            bool markedAssignmentVisible = false;
            for (const auto& assignment : departmentAssignment) {
                if (assignment.id == assignmentId) {
                    markedAssignmentVisible = assignment.submissionCount == 1 && assignment.markedCount == 1;
                }
            }
            expect(departmentAssignment.size() == 2 && markedAssignmentVisible,
                   "HoDs must see assignment and marked-submission counts for their department.");
            auto pending = database.submissionsForModeration(*hod);
            expect(pending.size() == 1 && pending.front().status == "MARKED",
                   "HoD must see lecturer-marked work in their department.");
            database.moderateSubmission(*hod, secondVersion, 75.0, "Moderation adjustment");
            expect(database.releasedMarks(*student).empty(), "Moderated results remain hidden until release.");
            expect(database.submissionsReadyForRelease(*hod).size() == 1,
                   "Moderated results must be available in the release queue.");
            database.releaseModerated(*hod, secondVersion);
            const auto released = database.releasedMarks(*student);
            expect(released.size() == 1 && released.front().finalMark == 75.0,
                   "The student should see only their released result.");
            const auto audit = database.auditForDepartment(*hod);
            expect(audit.size() == 2 && !audit.front().oldMark &&
                   audit.front().reason == "Initial mark" &&
                   audit.back().oldMark && audit.back().newMark == 75.0,
                   "Initial marking and moderation changes must both be audited.");

            const int overdueAssignment = database.createAssignment(*lecturer, courseId, "Overdue Test",
                                                                      "2000-01-01 00:00", 100.0);
            const SubmissionReceipt overdue = database.submit(*student, overdueAssignment, "overdue.txt");
            expect(overdue.status == "LATE" && overdue.daysLate > 3 && overdue.markWillBeZero,
                   "Submissions more than three days late must be recorded and marked for zero.");
            database.saveMark(*lecturer, overdue.id, 100.0, "Late submission", "Initial mark");
            auto overdueQueue = database.submissionsForModeration(*hod);
            expect(overdueQueue.size() == 1 && overdueQueue.front().finalMark == 0.0,
                   "Over-three-day work must receive a zero final mark.");

            database.updateUser(*admin, student->id(), student->username(), Role::Student,
                                student->departmentId(), false, "");
            expectFailure([&] { database.releasedMarks(*student); },
                          "Disabling an account must revoke its current session permissions.");
        }
        std::filesystem::remove_all(testRoot);
        std::cout << "All LMS workflow tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::filesystem::remove_all(testRoot);
        std::cerr << "Test failed: " << error.what() << '\n';
        return 1;
    }
}
