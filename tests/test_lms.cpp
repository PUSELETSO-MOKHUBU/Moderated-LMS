#include "Database.h"
#include "FileStorage.h"
#include "LatePenaltyStrategy.h"
#include "PasswordHasher.h"
#include "models/Assignment.h"
#include "models/Course.h"
#include "models/MarkAudit.h"
#include "models/Submission.h"
#include "models/User.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
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

std::string dateTimeOffsetFromNow(int minutes) {
    const std::time_t timestamp = std::time(nullptr) + minutes * 60;
    std::tm local = {};
#ifdef _WIN32
    localtime_s(&local, &timestamp);
#else
    localtime_r(&timestamp, &local);
#endif
    std::ostringstream output;
    output << std::put_time(&local, "%Y-%m-%d %H:%M");
    return output.str();
}
}

TEST_CASE("Late-penalty rules cover equivalence classes and boundaries", "[unit][penalty]") {
    StandardLatePenalty penalty;
    CHECK(penalty.penalty(100.0, 0) == Catch::Approx(0.0));
    CHECK(penalty.penalty(100.0, 1) == Catch::Approx(10.0));
    CHECK(penalty.penalty(100.0, 2) == Catch::Approx(20.0));
    CHECK(penalty.penalty(100.0, 3) == Catch::Approx(30.0));
    CHECK(penalty.penalty(100.0, 4) == Catch::Approx(30.0));
    CHECK(penalty.penalty(60.0, 1) == Catch::Approx(6.0));
    CHECK(penalty.penalty(0.0, 3) == Catch::Approx(0.0));

    CHECK(penalty.finalMark(80.0, 100.0, 1) == Catch::Approx(70.0));
    CHECK(penalty.finalMark(5.0, 100.0, 1) == Catch::Approx(0.0));
    CHECK(penalty.finalMark(100.0, 100.0, 3) == Catch::Approx(70.0));
    CHECK(penalty.finalMark(80.0, 100.0, 4) == Catch::Approx(0.0));
    CHECK_THROWS_AS(penalty.penalty(-1.0, 0), std::invalid_argument);
    CHECK_THROWS_AS(penalty.penalty(100.0, -1), std::invalid_argument);
    CHECK_THROWS_AS(penalty.penalty(std::numeric_limits<double>::infinity(), 1), std::invalid_argument);
    CHECK_THROWS_AS(penalty.finalMark(-1.0, 100.0, 0), std::invalid_argument);
    CHECK_THROWS_AS(penalty.finalMark(101.0, 100.0, 0), std::invalid_argument);
}

TEST_CASE("Role permissions and factory preserve role boundaries", "[unit][roles]") {
    Administrator admin(1, "admin", "hash", 1);
    Lecturer lecturer(2, "lecturer", "hash", 1);
    HeadOfDepartment hod(3, "hod", "hash", 1);
    Student student(4, "student", "hash", 1);

    CHECK(admin.role() == Role::Administrator);
    CHECK(admin.can(Permission::ManageUsers));
    CHECK(admin.can(Permission::ManageCourses));
    CHECK_FALSE(admin.can(Permission::TeachCourse));
    CHECK(lecturer.role() == Role::Lecturer);
    CHECK(lecturer.can(Permission::TeachCourse));
    CHECK_FALSE(lecturer.can(Permission::ManageUsers));
    CHECK(hod.role() == Role::HeadOfDepartment);
    CHECK(hod.can(Permission::ModerateDepartment));
    CHECK_FALSE(hod.can(Permission::ViewReleasedMarks));
    CHECK(student.role() == Role::Student);
    CHECK(student.can(Permission::SubmitWork));
    CHECK(student.can(Permission::ViewReleasedMarks));
    CHECK_FALSE(student.can(Permission::ManageCourses));
    CHECK(roleToText(Role::Administrator) == "ADMIN");
    CHECK(roleToText(Role::Lecturer) == "LECTURER");
    CHECK(roleToText(Role::HeadOfDepartment) == "HOD");
    CHECK(roleToText(Role::Student) == "STUDENT");
    CHECK_THROWS_AS(roleToText(static_cast<Role>(-1)), std::invalid_argument);

    CHECK(UserFactory::create(1, "admin", "hash", "ADMIN", 1)->role() == Role::Administrator);
    CHECK(UserFactory::create(2, "lecturer", "hash", "LECTURER", 1)->role() == Role::Lecturer);
    CHECK(UserFactory::create(3, "hod", "hash", "HOD", 1)->role() == Role::HeadOfDepartment);
    CHECK(UserFactory::create(4, "student", "hash", "STUDENT", 1)->role() == Role::Student);
    CHECK_THROWS_AS(UserFactory::create(5, "invalid", "hash", "UNKNOWN", 1), std::runtime_error);
}

TEST_CASE("Domain records initialize, expose, and update their values", "[unit][models]") {
    Course defaultCourse;
    CHECK(defaultCourse.id() == -1);
    CHECK(defaultCourse.departmentId() == -1);
    defaultCourse.setId(1);
    defaultCourse.setCode("CS4433");
    defaultCourse.setTitle("Software Engineering");
    defaultCourse.setDepartmentId(2);
    CHECK(defaultCourse.id() == 1);
    CHECK(defaultCourse.code() == "CS4433");
    CHECK(defaultCourse.title() == "Software Engineering");
    CHECK(defaultCourse.departmentId() == 2);
    const Course course(3, "CS4000", "Project", 4);
    CHECK(course.id() == 3);
    CHECK(course.code() == "CS4000");
    CHECK(course.title() == "Project");
    CHECK(course.departmentId() == 4);

    Assignment defaultAssignment;
    CHECK(defaultAssignment.id() == -1);
    CHECK(defaultAssignment.courseId() == -1);
    CHECK(defaultAssignment.maxMark() == Catch::Approx(0.0));
    defaultAssignment.setId(5);
    defaultAssignment.setCourseId(6);
    defaultAssignment.setTitle("Test");
    defaultAssignment.setDeadline("2099-12-31 23:59");
    defaultAssignment.setMaxMark(100.0);
    CHECK(defaultAssignment.id() == 5);
    CHECK(defaultAssignment.courseId() == 6);
    CHECK(defaultAssignment.title() == "Test");
    CHECK(defaultAssignment.deadline() == "2099-12-31 23:59");
    CHECK(defaultAssignment.maxMark() == Catch::Approx(100.0));
    const Assignment assignment(7, 8, "Final", "2099-10-10 10:00", 75.0);
    CHECK(assignment.id() == 7);
    CHECK(assignment.courseId() == 8);
    CHECK(assignment.title() == "Final");
    CHECK(assignment.deadline() == "2099-10-10 10:00");
    CHECK(assignment.maxMark() == Catch::Approx(75.0));

    Submission defaultSubmission;
    CHECK(defaultSubmission.id() == -1);
    CHECK(defaultSubmission.versionNo() == 1);
    CHECK(defaultSubmission.status() == SubmissionStatus::Submitted);
    CHECK(defaultSubmission.isActive());
    defaultSubmission.setId(9);
    defaultSubmission.setAssignmentId(10);
    defaultSubmission.setStudentId(11);
    defaultSubmission.setVersionNo(2);
    defaultSubmission.setSubmittedAt("2026-10-31 10:00");
    defaultSubmission.setStatus(SubmissionStatus::Marked);
    defaultSubmission.setFilePath("uploads/submission.pdf");
    defaultSubmission.setIsActive(false);
    CHECK(defaultSubmission.id() == 9);
    CHECK(defaultSubmission.assignmentId() == 10);
    CHECK(defaultSubmission.studentId() == 11);
    CHECK(defaultSubmission.versionNo() == 2);
    CHECK(defaultSubmission.submittedAt() == "2026-10-31 10:00");
    CHECK(defaultSubmission.status() == SubmissionStatus::Marked);
    CHECK(defaultSubmission.filePath() == "uploads/submission.pdf");
    CHECK_FALSE(defaultSubmission.isActive());
    const Submission submission(12, 13, 14, 3, "2026-10-30 10:00",
                                SubmissionStatus::Released, "released.pdf", false);
    CHECK(submission.id() == 12);
    CHECK(submission.assignmentId() == 13);
    CHECK(submission.studentId() == 14);
    CHECK(submission.versionNo() == 3);
    CHECK(submission.submittedAt() == "2026-10-30 10:00");
    CHECK(submission.status() == SubmissionStatus::Released);
    CHECK(submission.filePath() == "released.pdf");
    CHECK_FALSE(submission.isActive());

    MarkAudit defaultAudit;
    CHECK(defaultAudit.id() == -1);
    CHECK(defaultAudit.oldMark() == Catch::Approx(0.0));
    defaultAudit.setId(15);
    defaultAudit.setSubmissionId(16);
    defaultAudit.setChangedBy(17);
    defaultAudit.setOldMark(60.0);
    defaultAudit.setNewMark(70.0);
    defaultAudit.setReason("Moderation");
    defaultAudit.setChangedAt("2026-10-31 11:00");
    CHECK(defaultAudit.id() == 15);
    CHECK(defaultAudit.submissionId() == 16);
    CHECK(defaultAudit.changedBy() == 17);
    CHECK(defaultAudit.oldMark() == Catch::Approx(60.0));
    CHECK(defaultAudit.newMark() == Catch::Approx(70.0));
    CHECK(defaultAudit.reason() == "Moderation");
    CHECK(defaultAudit.changedAt() == "2026-10-31 11:00");
    const MarkAudit audit(18, 19, 20, 50.0, 55.0, "Review", "2026-10-31 12:00");
    CHECK(audit.id() == 18);
    CHECK(audit.submissionId() == 19);
    CHECK(audit.changedBy() == 20);
    CHECK(audit.oldMark() == Catch::Approx(50.0));
    CHECK(audit.newMark() == Catch::Approx(55.0));
    CHECK(audit.reason() == "Review");
    CHECK(audit.changedAt() == "2026-10-31 12:00");
}

TEST_CASE("Password hashing rejects invalid input and verifies encoded hashes", "[unit][password]") {
    const std::string hash = PasswordHasher::hash("correct horse");
    CHECK(PasswordHasher::isEncodedHash(hash));
    CHECK(hash != "correct horse");
    CHECK(PasswordHasher::verify("correct horse", hash));
    CHECK_FALSE(PasswordHasher::verify("wrong", hash));
    CHECK_FALSE(PasswordHasher::verify("", hash));
    CHECK_FALSE(PasswordHasher::verify("correct horse", "plaintext"));
    CHECK_FALSE(PasswordHasher::isEncodedHash("pbkdf2_sha256$bad$invalid$invalid"));
    CHECK(PasswordHasher::hash("correct horse") != hash);
    CHECK_THROWS_AS(PasswordHasher::hash(""), std::invalid_argument);
}

TEST_CASE("Document storage enforces supported formats and size boundaries", "[unit][files]") {
    const auto testRoot = std::filesystem::temp_directory_path() /
        ("moderated_lms_file_test_" + std::to_string(
            std::chrono::high_resolution_clock::now().time_since_epoch().count()));
    const auto source = testRoot / "maximum.pdf";
    const auto oversized = testRoot / "oversized.pdf";
    const auto empty = testRoot / "empty.pdf";
    std::filesystem::create_directories(testRoot);
    try {
        std::ofstream(source, std::ios::binary).close();
        std::filesystem::resize_file(source, FileStorage::maximumFileBytes());
        std::ofstream(oversized, std::ios::binary).close();
        std::filesystem::resize_file(oversized, FileStorage::maximumFileBytes() + 1);
        std::ofstream(empty, std::ios::binary).close();

        FileStorage storage(testRoot / "uploads");
        const auto stored = storage.storeDocument(source.string(), UploadKind::Submission);
        CHECK(std::filesystem::file_size(stored) == FileStorage::maximumFileBytes());
        CHECK(std::filesystem::path(stored).parent_path().filename() == "submissions");
        CHECK_THROWS_AS(storage.storeDocument(oversized.string(), UploadKind::Submission), std::invalid_argument);
        CHECK_THROWS_AS(storage.storeDocument(empty.string(), UploadKind::Submission), std::invalid_argument);
        CHECK_THROWS_AS(storage.storeDocument("  ", UploadKind::Submission), std::invalid_argument);
        CHECK(FileStorage::isSupportedDocument("brief.DOCX"));
        CHECK_FALSE(FileStorage::isSupportedDocument("program.exe"));
        CHECK_THROWS_AS(storage.removeStoredFile((testRoot / "outside.pdf").string()), std::invalid_argument);
        storage.removeStoredFile(stored);
        CHECK_FALSE(std::filesystem::exists(stored));
    } catch (...) {
        std::filesystem::remove_all(testRoot);
        throw;
    }
    std::filesystem::remove_all(testRoot);
}

TEST_CASE("Role-scoped LMS workflow persists and releases moderated results",
          "[integration][system][acceptance]") {
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
        expectFailure([&] {
            fileStorage.openDocument((testRoot / "missing.pdf").string());
        }, "The document viewer must report missing documents.");

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
            expectFailure([&] {
                database.createDepartment(*lecturer, "Forbidden Department");
            }, "Lecturers must not manage departments.");
            expectFailure([&] {
                database.createUser(*admin, "weak-password", "short", Role::Student, -1);
            }, "New accounts must enforce the minimum password length.");
            expectFailure([&] {
                database.createUser(*admin, "unassigned-hod", "password123", Role::HeadOfDepartment, -1);
            }, "HoD accounts must be assigned to a department.");

            const int newDepartmentId = database.createDepartment(*admin, "Engineering");
            const int newLecturerId = database.createUser(*admin, "lecturer2", "lecturer2-password",
                                                            Role::Lecturer, newDepartmentId);
            const int newStudentId = database.createUser(*admin, "student2", "student2-password",
                                                           Role::Student, newDepartmentId);
            const int newHodId = database.createUser(*admin, "hod2", "hod2-password",
                                                       Role::HeadOfDepartment, newDepartmentId);
            const int newCourseId = database.createCourse(*admin, "ENG100", "Engineering Fundamentals",
                                                           newDepartmentId);
            database.assignLecturer(*admin, newLecturerId, newCourseId);
            database.enrolStudent(*admin, newStudentId, newCourseId);
            database.updateDepartment(*admin, newDepartmentId, "Applied Engineering");
            database.updateCourse(*admin, newCourseId, "ENG101", "Applied Engineering Fundamentals",
                                  newDepartmentId);
            database.updateUser(*admin, newLecturerId, "lecturer2-updated", Role::Lecturer,
                                newDepartmentId, true, "updated-password");
            database.updateUser(*admin, newStudentId, "student2-updated", Role::Student,
                                newDepartmentId, true, "");
            const auto updatedLecturer = database.authenticate("lecturer2-updated", "updated-password");
            const auto updatedStudent = database.authenticate("student2-updated", "student2-password");
            const auto otherHod = database.authenticate("hod2", "hod2-password");
            expect(updatedLecturer && updatedStudent && otherHod,
                   "Administrator account edits and password changes must persist.");
            const auto departments = database.listDepartments(*admin);
            const auto users = database.listUsers(*admin);
            const auto adminCourses = database.coursesFor(*admin);
            expect(departments.size() == 2 && users.size() == 7 && adminCourses.size() == 2,
                   "Administrators must list all persisted departments, users, and courses.");
            expect(database.coursesFor(*updatedLecturer).size() == 1 &&
                   database.coursesFor(*updatedStudent).size() == 1 &&
                   database.coursesFor(*hod).size() == 1 &&
                   database.coursesFor(*otherHod).size() == 1,
                   "Course visibility must be scoped to each lecturer, student, and HoD.");
            expectFailure([&] {
                database.assignLecturer(*admin, newLecturerId, newCourseId);
            }, "Duplicate course assignments must be rejected.");
            expectFailure([&] {
                database.enrolStudent(*admin, newStudentId, newCourseId);
            }, "Duplicate student enrolments must be rejected.");

            const auto lecturerCourses = database.coursesFor(*lecturer);
            expect(lecturerCourses.size() == 1, "Lecturer should only see assigned courses.");
            const auto hodCourses = database.coursesFor(*hod);
            expect(hodCourses.size() == 1 && hodCourses.front().id == lecturerCourses.front().id,
                   "The demo HoD must be assigned to the General department and see its courses.");
            const int courseId = lecturerCourses.front().id;
            const int assignmentId = database.createAssignment(*lecturer, courseId, "Integration Test",
                                                                 "2099-12-31 23:59", 100.0, storedBrief);
            expect(database.assignmentsForDepartment(*otherHod).empty(),
                   "A HoD must not see assignments in another department.");
            expectFailure([&] {
                database.createAssignment(*lecturer, courseId, "Invalid date", "2099-02-30 12:00", 100.0);
            }, "Impossible calendar dates must be rejected.");
            expectFailure([&] {
                database.createAssignment(*lecturer, courseId, "Invalid maximum", "2099-12-31 23:59", 0.0);
            }, "Assignment maximum marks must be positive.");
            const auto departmentAssignments = database.assignmentsForDepartment(*hod);
            expect(departmentAssignments.size() == 1 &&
                   departmentAssignments.front().attachmentPath == storedBrief,
                   "HoDs must see department assignments and their uploaded briefs.");
            const int extensionAssignment = database.createAssignment(*lecturer, courseId, "Extension Test",
                                                                        "2099-11-30 23:59", 100.0);
            database.grantExtension(*lecturer, extensionAssignment, student->id(),
                                    "2099-12-15 23:59", "Approved extension");
            expectFailure([&] {
                database.grantExtension(*lecturer, extensionAssignment, student->id(),
                                        "2099-11-01 23:59", "Earlier deadline");
            }, "An extension must be later than the original deadline.");
            bool extensionFound = false;
            for (const auto& assignment : database.assignmentsForStudent(*student)) {
                if (assignment.id == extensionAssignment) {
                    extensionFound = assignment.effectiveDeadline == "2099-12-15T23:59:00";
                }
            }
            expect(extensionFound, "Individual extension must replace the student's effective deadline.");
            const auto studentCanSeeAssignment = [&database, extensionAssignment](const User& candidate) {
                for (const auto& assignment : database.assignmentsForStudent(candidate)) {
                    if (assignment.id == extensionAssignment) return true;
                }
                return false;
            };
            expect(!studentCanSeeAssignment(*updatedStudent),
                   "An assignment must not be visible to an unenrolled student.");
            expectFailure([&] {
                database.grantClassExtension(*updatedLecturer, extensionAssignment,
                                             "2099-12-20 23:59", "Wrong course");
            }, "A lecturer must not extend an assignment outside their assigned courses.");
            expectFailure([&] {
                database.grantClassExtension(*student, extensionAssignment,
                                             "2099-12-20 23:59", "Wrong role");
            }, "A student must not grant a class extension.");
            expectFailure([&] {
                database.grantClassExtension(*lecturer, extensionAssignment,
                                             "2099-11-01 23:59", "Earlier deadline");
            }, "A class extension must be later than the original deadline.");
            database.grantClassExtension(*lecturer, extensionAssignment,
                                         "2099-12-20 23:59", "Approved class extension");
            expect(!studentCanSeeAssignment(*updatedStudent),
                   "An unenrolled student must not see a class extension.");
            database.enrolStudent(*admin, updatedStudent->id(), courseId);
            const auto classAssignments = database.assignmentsForStudent(*updatedStudent);
            bool classDeadlineVisible = false;
            for (const auto& assignment : classAssignments) {
                if (assignment.id == extensionAssignment) {
                    classDeadlineVisible = assignment.effectiveDeadline == "2099-12-20T23:59:00";
                }
            }
            expect(classDeadlineVisible,
                   "A newly enrolled student must see the active class-wide extension.");
            database.grantExtension(*lecturer, extensionAssignment, student->id(),
                                    "2099-12-30 23:59", "Longer individual extension");
            bool individualDeadlineWins = false;
            for (const auto& assignment : database.assignmentsForStudent(*student)) {
                if (assignment.id == extensionAssignment) {
                    individualDeadlineWins = assignment.effectiveDeadline == "2099-12-30T23:59:00";
                }
            }
            expect(individualDeadlineWins,
                   "The later individual deadline must remain effective over the class deadline.");
            expectFailure([&] {
                database.grantClassExtension(*lecturer, extensionAssignment,
                                             "2099-12-19 23:59", "Shorter class deadline");
            }, "Updating a class extension must not shorten its existing deadline.");

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
            const auto studentSubmissions = database.submissionsForStudent(*student);
            expect(studentSubmissions.size() == 2 &&
                   studentSubmissions.back().studentId == student->id() &&
                   studentSubmissions.back().filePath == secondDocument,
                   "Students must be able to inspect their own stored submission versions.");

            database.saveMark(*lecturer, secondVersion, 80.0, "Good work", "ignored initial reason");
            expectFailure([&] {
                database.grantClassExtension(*lecturer, assignmentId,
                                             "2100-01-15 23:59", "After marking");
            }, "A class extension must not change an already marked result.");
            expect(database.submissionsForModeration(*otherHod).empty(),
                   "A HoD must not see marked work outside their department.");
            expectFailure([&] {
                database.moderateSubmission(*otherHod, secondVersion, 50.0, "Cross-department change");
            }, "A HoD must not moderate work outside their department.");
            expectFailure([&] {
                database.saveMark(*lecturer, secondVersion, 101.0, "Invalid mark", "Invalid mark");
            }, "A mark above the assignment maximum must be rejected.");
            expectFailure([&] {
                database.grantExtension(*lecturer, assignmentId, student->id(),
                                        "2099-12-30 23:59", "After marking");
            }, "A marked submission must not have its extension changed.");
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
            expect(database.submissionsReadyForRelease(*hod).empty(),
                   "Marked work must not appear in the release queue before moderation.");
            database.moderateSubmission(*hod, secondVersion, 75.0, "Moderation adjustment");
            expect(database.releasedMarks(*student).empty(), "Moderated results remain hidden until release.");
            expect(database.submissionsForModeration(*hod).empty(),
                   "Moderated submissions must leave the pending moderation queue.");
            expect(database.submissionsReadyForRelease(*hod).size() == 1,
                   "Moderated results must be available in the release queue.");
            database.releaseModerated(*hod, secondVersion);
            const auto released = database.releasedMarks(*student);
            expect(released.size() == 1 && released.front().finalMark == 75.0,
                   "The student should see only their released result.");
            expect(database.releasedMarks(*updatedStudent).empty(),
                   "A student must not see another student's released result.");
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
            expectFailure([&] {
                database.submit(*student, overdueAssignment, "late-resubmission.txt");
            }, "Resubmission after the effective deadline must be rejected.");
            database.saveMark(*lecturer, overdue.id, 100.0, "Late submission", "Initial mark");
            auto overdueQueue = database.submissionsForModeration(*hod);
            expect(overdueQueue.size() == 1 && overdueQueue.front().finalMark == 0.0,
                   "Over-three-day work must receive a zero final mark.");
            database.moderateSubmission(*hod, overdue.id, std::nullopt, "");
            database.releaseModerated(*hod, overdue.id);

            const int nearFutureAssignment = database.createAssignment(
                *updatedLecturer, newCourseId, "Deadline Boundary", dateTimeOffsetFromNow(5), 100.0);
            expectFailure([&] {
                database.submit(*student, nearFutureAssignment, "not-enrolled.pdf");
            }, "A student not enrolled in the course must not submit its assignment.");
            const SubmissionReceipt beforeDeadline =
                database.submit(*updatedStudent, nearFutureAssignment, "before-deadline.pdf");
            expect(beforeDeadline.status == "SUBMITTED" && beforeDeadline.daysLate == 0 &&
                   beforeDeadline.penalty == 0.0,
                   "A submission before the deadline must not receive a late penalty.");

            const int nearPastAssignment = database.createAssignment(
                *updatedLecturer, newCourseId, "Late Boundary", dateTimeOffsetFromNow(-1), 100.0);
            const SubmissionReceipt justLate =
                database.submit(*updatedStudent, nearPastAssignment, "just-late.pdf");
            expect(justLate.status == "LATE" && justLate.daysLate == 1 &&
                   justLate.penalty == 10.0,
                   "A submission just after the deadline must enter the one-day late partition.");

            const int classOverdueAssignment = database.createAssignment(
                *lecturer, courseId, "Class Extension Submission", "2000-01-01 00:00", 100.0);
            const SubmissionReceipt beforeClassExtension =
                database.submit(*student, classOverdueAssignment, "class-extension-before.txt");
            expect(beforeClassExtension.status == "LATE",
                   "A submission past the original class deadline must initially be late.");
            database.grantClassExtension(*lecturer, classOverdueAssignment,
                                         dateTimeOffsetFromNow(60), "Approved after-class extension");
            database.grantExtension(*lecturer, classOverdueAssignment, student->id(),
                                    "2000-01-02 00:00", "Individual date before class deadline");
            bool submissionStatusRefreshed = false;
            for (const auto& submission : database.submissionsForLecturer(*lecturer)) {
                if (submission.id == beforeClassExtension.id) {
                    submissionStatusRefreshed = submission.status == "SUBMITTED";
                }
            }
            expect(submissionStatusRefreshed,
                   "A class extension must refresh active submission status against the new effective deadline.");
            const SubmissionReceipt afterClassExtension =
                database.submit(*student, classOverdueAssignment, "class-extension-after.txt");
            expect(afterClassExtension.status == "SUBMITTED" && afterClassExtension.daysLate == 0 &&
                   afterClassExtension.version == 2,
                   "A class-wide effective deadline must permit a timely resubmission after the original deadline.");
            database.saveMark(*lecturer, afterClassExtension.id, 80.0,
                              "Class deadline applied", "Initial mark");
            bool classDeadlineAppliedToMark = false;
            for (const auto& submission : database.submissionsForLecturer(*lecturer)) {
                if (submission.id == afterClassExtension.id) {
                    classDeadlineAppliedToMark = submission.penalty == 0.0;
                }
            }
            expect(classDeadlineAppliedToMark,
                   "Mark calculation must use the class-wide effective deadline.");

            database.updateUser(*admin, student->id(), student->username(), Role::Student,
                                student->departmentId(), false, "");
            expectFailure([&] { database.releasedMarks(*student); },
                          "Disabling an account must revoke its current session permissions.");
        }
        std::filesystem::remove_all(testRoot);
        std::cout << "All LMS workflow tests passed.\n";
    } catch (const std::exception& error) {
        std::filesystem::remove_all(testRoot);
        FAIL("Test failed: " << error.what());
    }
}
