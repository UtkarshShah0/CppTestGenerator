// File: test_DepartmentsController.cpp
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <drogon/HttpController.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <drogon/app.h>
#include "../DepartmentsController.h"
#include "../models/Department.h"
#include "../models/Person.h"
#include "../utils/utils.h"
#include <memory>
#include <string>
#include <vector>

using namespace drogon;
using namespace drogon::orm;
using namespace drogon_model::org_chart;
using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;

class MockDbClient : public drogon::orm::DbClient {
public:
    MOCK_METHOD2(execSqlAsync, void(const std::string&, std::function<void(const drogon::orm::Result&)>&&));
    MOCK_METHOD3(execSqlAsync, void(const std::string&, std::function<void(const drogon::orm::Result&)>&&, std::function<void(const std::exception_ptr&)>&&));
    MOCK_METHOD2(execSqlSync, drogon::orm::Result(const std::string&, const std::vector<std::string>&));
    MOCK_METHOD0(newTransaction, std::shared_ptr<drogon::orm::Transaction>());
    MOCK_METHOD1(newTransactionAsync, void(std::function<void(std::shared_ptr<drogon::orm::Transaction>)>&&));
};

class DepartmentsControllerTest : public ::testing::Test {
protected:
    void SetUp() override {
        app().registerSyncAdvice([](const HttpRequestPtr& req) { return req; });
        controller = std::make_shared<DepartmentsController>();
        mockDbClient = std::make_shared<MockDbClient>();
        drogon::app().registerDbClient(mockDbClient);
    }

    void TearDown() override {
        drogon::app().clearDbClients();
    }

    std::shared_ptr<DepartmentsController> controller;
    std::shared_ptr<MockDbClient> mockDbClient;
};

TEST_F(DepartmentsControllerTest, GetDepartments_Success) {
    auto req = HttpRequest::newHttpRequest();
    req->setPath("/departments");
    req->setParameter("offset", "0");
    req->setParameter("limit", "10");
    req->setParameter("sort_field", "id");
    req->setParameter("sort_order", "asc");

    std::vector<Department> mockDepartments;
    Department dept;
    dept.setId(1);
    dept.setName("Test Dept");
    mockDepartments.push_back(dept);

    EXPECT_CALL(*mockDbClient, execSqlAsync(_, _, _))
        .WillOnce(Invoke([&mockDepartments](const std::string&, std::function<void(const drogon::orm::Result&)>&& callback, std::function<void(const std::exception_ptr&)>&&) {
            drogon::orm::Result result(nullptr);
            callback(result);
        }));

    bool callbackCalled = false;
    auto callback = [&callbackCalled](const HttpResponsePtr& resp) {
        EXPECT_EQ(resp->getStatusCode(), HttpStatusCode::k200OK);
        callbackCalled = true;
    };

    controller->get(req, callback);
    EXPECT_TRUE(callbackCalled);
}

TEST_F(DepartmentsControllerTest, GetDepartments_DatabaseError) {
    auto req = HttpRequest::newHttpRequest();
    req->setPath("/departments");

    EXPECT_CALL(*mockDbClient, execSqlAsync(_, _, _))
        .WillOnce(Invoke([](const std::string&, std::function<void(const drogon::orm::Result&)>&&, std::function<void(const std::exception_ptr&)>&& errorCallback) {
            errorCallback(std::make_exception_ptr(DrogonDbException("Database error")));
        }));

    bool callbackCalled = false;
    auto callback = [&callbackCalled](const HttpResponsePtr& resp) {
        EXPECT_EQ(resp->getStatusCode(), HttpStatusCode::k500InternalServerError);
        callbackCalled = true;
    };

    controller->get(req, callback);
    EXPECT_TRUE(callbackCalled);
}

TEST_F(DepartmentsControllerTest, GetOneDepartment_Success) {
    auto req = HttpRequest::newHttpRequest();
    req->setPath("/departments/1");

    Department mockDept;
    mockDept.setId(1);
    mockDept.setName("Test Dept");

    EXPECT_CALL(*mockDbClient, execSqlAsync(_, _, _))
        .WillOnce(Invoke([&mockDept](const std::string&, std::function<void(const drogon::orm::Result&)>&& callback, std::function<void(const std::exception_ptr&)>&&) {
            drogon::orm::Result result(nullptr);
            callback(result);
        }));

    bool callbackCalled = false;
    auto callback = [&callbackCalled](const HttpResponsePtr& resp) {
        EXPECT_EQ(resp->getStatusCode(), HttpStatusCode::k201Created);
        callbackCalled = true;
    };

    controller->getOne(req, callback, 1);
    EXPECT_TRUE(callbackCalled);
}

TEST_F(DepartmentsControllerTest, GetOneDepartment_NotFound) {
    auto req = HttpRequest::newHttpRequest();
    req->setPath("/departments/1");

    EXPECT_CALL(*mockDbClient, execSqlAsync(_, _, _))
        .WillOnce(Invoke([](const std::string&, std::function<void(const drogon::orm::Result&)>&&, std::function<void(const std::exception_ptr&)>&& errorCallback) {
            errorCallback(std::make_exception_ptr(drogon::orm::UnexpectedRows("Not found")));
        }));

    bool callbackCalled = false;
    auto callback = [&callbackCalled](const HttpResponsePtr& resp) {
        EXPECT_EQ(resp->getStatusCode(), HttpStatusCode::k404NotFound);
        callbackCalled = true;
    };

    controller->getOne(req, callback, 1);
    EXPECT_TRUE(callbackCalled);
}

TEST_F(DepartmentsControllerTest, CreateOneDepartment_Success) {
    auto req = HttpRequest::newHttpRequest();
    req->setPath("/departments");
    req->setMethod(Post);

    Department newDept;
    newDept.setName("New Dept");

    EXPECT_CALL(*mockDbClient, execSqlAsync(_, _, _))
        .WillOnce(Invoke([](const std::string&, std::function<void(const drogon::orm::Result&)>&& callback, std::function<void(const std::exception_ptr&)>&&) {
            drogon::orm::Result result(nullptr);
            callback(result);
        }));

    bool callbackCalled = false;
    auto callback = [&callbackCalled](const HttpResponsePtr& resp) {
        EXPECT_EQ(resp->getStatusCode(), HttpStatusCode::k201Created);
        callbackCalled = true;
    };

    controller->createOne(req, callback, std::move(newDept));
    EXPECT_TRUE(callbackCalled);
}

TEST_F(DepartmentsControllerTest, CreateOneDepartment_DatabaseError) {
    auto req = HttpRequest::newHttpRequest();
    req->setPath("/departments");
    req->setMethod(Post);

    Department newDept;
    newDept.setName("New Dept");

    EXPECT_CALL(*mockDbClient, execSqlAsync(_, _, _))
        .WillOnce(Invoke([](const std::string&, std::function<void(const drogon::orm::Result&)>&&, std::function<void(const std::exception_ptr&)>&& errorCallback) {
            errorCallback(std::make_exception_ptr(DrogonDbException("Database error")));
        }));

    bool callbackCalled = false;
    auto callback = [&callbackCalled](const HttpResponsePtr& resp) {
        EXPECT_EQ(resp->getStatusCode(), HttpStatusCode::k500InternalServerError);
        callbackCalled = true;
    };

    controller->createOne(req, callback, std::move(newDept));
    EXPECT_TRUE(callbackCalled);
}

TEST_F(DepartmentsControllerTest, UpdateOneDepartment_Success) {
    auto req = HttpRequest::newHttpRequest();
    req->setPath("/departments/1");
    req->setMethod(Put);

    Department updateDept;
    updateDept.setName("Updated Dept");

    EXPECT_CALL(*mockDbClient, execSqlSync(_, _))
        .WillOnce(Return(drogon::orm::Result(nullptr)));
    EXPECT_CALL(*mockDbClient, execSqlAsync(_, _, _))
        .WillOnce(Invoke([](const std::string&, std::function<void(const drogon::orm::Result&)>&& callback, std::function<void(const std::exception_ptr&)>&&) {
            drogon::orm::Result result(nullptr);
            callback(result);
        }));

    bool callbackCalled = false;
    auto callback = [&callbackCalled](const HttpResponsePtr& resp) {
        EXPECT_EQ(resp->getStatusCode(), HttpStatusCode::k204NoContent);
        callbackCalled = true;
    };

    controller->updateOne(req, callback, 1, std::move(updateDept));
    EXPECT_TRUE(callbackCalled);
}

TEST_F(DepartmentsControllerTest, UpdateOneDepartment_NotFound) {
    auto req = HttpRequest::newHttpRequest();
    req->setPath("/departments/1");
    req->setMethod(Put);

    Department updateDept;
    updateDept.setName("Updated Dept");

    EXPECT_CALL(*mockDbClient, execSqlSync(_, _))
        .WillOnce(Invoke([](const std::string&, const std::vector<std::string>&) -> drogon::orm::Result {
            throw DrogonDbException("Not found");
        }));

    bool callbackCalled = false;
    auto callback = [&callbackCalled](const HttpResponsePtr& resp) {
        EXPECT_EQ(resp->getStatusCode(), HttpStatusCode::k404NotFound);
        callbackCalled = true;
    };

    controller->updateOne(req, callback, 1, std::move(updateDept));
    EXPECT_TRUE(callbackCalled);
}

TEST_F(DepartmentsControllerTest, DeleteOneDepartment_Success) {
    auto req = HttpRequest::newHttpRequest();
    req->setPath("/departments/1");
    req->setMethod(Delete);

    EXPECT_CALL(*mockDbClient, execSqlAsync(_, _, _))
        .WillOnce(Invoke([](const std::string&, std::function<void(const drogon::orm::Result&)>&& callback, std::function<void(const std::exception_ptr&)>&&) {
            drogon::orm::Result result(nullptr);
            callback(result);
        }));

    bool callbackCalled = false;
    auto callback = [&callbackCalled](const HttpResponsePtr& resp) {
        EXPECT_EQ(resp->getStatusCode(), HttpStatusCode::k204NoContent);
        callbackCalled = true;
    };

    controller->deleteOne(req, callback, 1);
    EXPECT_TRUE(callbackCalled);
}

TEST_F(DepartmentsControllerTest, DeleteOneDepartment_DatabaseError) {
    auto req = HttpRequest::newHttpRequest();
    req->setPath("/departments/1");
    req->setMethod(Delete);

    EXPECT_CALL(*mockDbClient, execSqlAsync(_, _, _))
        .WillOnce(Invoke([](const std::string&, std::function<void(const drogon::orm::Result&)>&&, std::function<void(const std::exception_ptr&)>&& errorCallback) {
            errorCallback(std::make_exception_ptr(DrogonDbException("Database error")));
        }));

    bool callbackCalled = false;
    auto callback = [&callbackCalled](const HttpResponsePtr& resp) {
        EXPECT_EQ(resp->getStatusCode(), HttpStatusCode::k500InternalServerError);
        callbackCalled = true;
    };

    controller->deleteOne(req, callback, 1);
    EXPECT_TRUE(callbackCalled);
}

TEST_F(DepartmentsControllerTest, GetDepartmentPersons_Success) {
    auto req = HttpRequest::newHttpRequest();
    req->setPath("/departments/1/persons");

    EXPECT_CALL(*mockDbClient, execSqlSync(_, _))
        .WillOnce(Return(drogon::orm::Result(nullptr)));
    EXPECT_CALL(*mockDbClient, execSqlAsync(_, _, _))
        .WillOnce(Invoke([](const std::string&, std::function<void(const drogon::orm::Result&)>&& callback, std::function<void(const std::exception_ptr&)>&&) {
            drogon::orm::Result result(nullptr);
            callback(result);
        }));

    bool callbackCalled = false;
    auto callback = [&callbackCalled](const HttpResponsePtr& resp) {
        EXPECT_EQ(resp->getStatusCode(), HttpStatusCode::k200OK);
        callbackCalled = true;
    };

    controller->getDepartmentPersons(req, callback, 1);
    EXPECT_TRUE(callbackCalled);
}

TEST_F(DepartmentsControllerTest, GetDepartmentPersons_NotFound) {
    auto req = HttpRequest::newHttpRequest();
    req->setPath("/departments/1/persons");

    EXPECT_CALL(*mockDbClient, execSqlSync(_, _))
        .WillOnce(Invoke([](const std::string&, const std::vector<std::string>&) -> drogon::orm::Result {
            throw DrogonDbException("Not found");
        }));

    bool callbackCalled = false;
    auto callback = [&callbackCalled](const HttpResponsePtr& resp) {
        EXPECT_EQ(resp->getStatusCode(), HttpStatusCode::k404NotFound);
        callbackCalled = true;
    };

    controller->getDepartmentPersons(req, callback, 1);
    EXPECT_TRUE(callbackCalled);
}