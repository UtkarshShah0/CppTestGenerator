// File: test_JobsController.cpp
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <drogon/app.h>
#include "../JobsController.h"
#include "../models/Job.h"
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

class MockDbClient : public drogon::orm::DbClient {
public:
    MOCK_METHOD(void, execSqlAsync, (const std::string&, std::function<void(const drogon::orm::Result&)>, std::function<void(const std::exception_ptr&)>, const std::vector<std::string>&), (override));
    MOCK_METHOD(std::shared_future<drogon::orm::Result>, execSqlAsyncFuture, (const std::string&, const std::vector<std::string>&), (override));
    MOCK_METHOD(void, execSqlSync, (const std::string&, std::function<void(const drogon::orm::Result&)>, std::function<void(const std::exception_ptr&)>, const std::vector<std::string>&), (override));
    MOCK_METHOD(bool, hasAvailableConnections, (), (const, override));
    MOCK_METHOD(std::string, type, (), (const, override));
    MOCK_METHOD(void, newTransactionAsync, (std::function<void(std::shared_ptr<drogon::orm::Transaction>)>), (override));
    MOCK_METHOD(std::shared_ptr<drogon::orm::Transaction>, newTransaction, (), (override));
};

class JobsControllerTest : public ::testing::Test {
protected:
    void SetUp() override {
        app().registerSyncAdvice([](const drogon::HttpRequestPtr&) { return true; });
        mockDbClient = std::make_shared<MockDbClient>();
        drogon::app().registerDbClient(mockDbClient);
    }

    void TearDown() override {
        drogon::app().clearDbClients();
    }

    std::shared_ptr<MockDbClient> mockDbClient;
    JobsController controller;
};

TEST_F(JobsControllerTest, GetJobs_Success) {
    auto req = HttpRequest::newHttpRequest();
    req->setPath("/jobs");
    req->setMethod(Get);

    std::vector<Job> mockJobs = {Job(), Job()};
    EXPECT_CALL(*mockDbClient, execSqlAsync(_, _, _, _))
        .WillOnce(Invoke([&mockJobs](const std::string&, std::function<void(const drogon::orm::Result&)> callback, std::function<void(const std::exception_ptr&)>, const std::vector<std::string>&) {
            callback(drogon::orm::Result());
        }));

    bool called = false;
    auto callback = [&called, &mockJobs](const HttpResponsePtr& resp) {
        called = true;
        EXPECT_EQ(resp->getStatusCode(), HttpStatusCode::k200OK);
        auto json = resp->getJsonObject();
        EXPECT_TRUE(json);
        EXPECT_EQ(json->size(), mockJobs.size());
    };

    controller.get(req, std::move(callback));
    EXPECT_TRUE(called);
}

TEST_F(JobsControllerTest, GetJobs_DatabaseError) {
    auto req = HttpRequest::newHttpRequest();
    req->setPath("/jobs");
    req->setMethod(Get);

    EXPECT_CALL(*mockDbClient, execSqlAsync(_, _, _, _))
        .WillOnce(Invoke([](const std::string&, std::function<void(const drogon::orm::Result&)>, std::function<void(const std::exception_ptr&)> errorCallback, const std::vector<std::string>&) {
            errorCallback(std::make_exception_ptr(std::runtime_error("DB Error")));
        }));

    bool called = false;
    auto callback = [&called](const HttpResponsePtr& resp) {
        called = true;
        EXPECT_EQ(resp->getStatusCode(), HttpStatusCode::k500InternalServerError);
    };

    controller.get(req, std::move(callback));
    EXPECT_TRUE(called);
}

TEST_F(JobsControllerTest, GetOneJob_Success) {
    auto req = HttpRequest::newHttpRequest();
    req->setPath("/jobs/1");
    req->setMethod(Get);

    Job mockJob;
    EXPECT_CALL(*mockDbClient, execSqlAsync(_, _, _, _))
        .WillOnce(Invoke([&mockJob](const std::string&, std::function<void(const drogon::orm::Result&)> callback, std::function<void(const std::exception_ptr&)>, const std::vector<std::string>&) {
            callback(drogon::orm::Result());
        }));

    bool called = false;
    auto callback = [&called](const HttpResponsePtr& resp) {
        called = true;
        EXPECT_EQ(resp->getStatusCode(), HttpStatusCode::k201Created);
        EXPECT_TRUE(resp->getJsonObject());
    };

    controller.getOne(req, std::move(callback), 1);
    EXPECT_TRUE(called);
}

TEST_F(JobsControllerTest, GetOneJob_NotFound) {
    auto req = HttpRequest::newHttpRequest();
    req->setPath("/jobs/1");
    req->setMethod(Get);

    EXPECT_CALL(*mockDbClient, execSqlAsync(_, _, _, _))
        .WillOnce(Invoke([](const std::string&, std::function<void(const drogon::orm::Result&)>, std::function<void(const std::exception_ptr&)> errorCallback, const std::vector<std::string>&) {
            errorCallback(std::make_exception_ptr(drogon::orm::UnexpectedRows("Not found")));
        }));

    bool called = false;
    auto callback = [&called](const HttpResponsePtr& resp) {
        called = true;
        EXPECT_EQ(resp->getStatusCode(), HttpStatusCode::k404NotFound);
    };

    controller.getOne(req, std::move(callback), 1);
    EXPECT_TRUE(called);
}

TEST_F(JobsControllerTest, CreateOneJob_Success) {
    auto req = HttpRequest::newHttpRequest();
    req->setPath("/jobs");
    req->setMethod(Post);
    Job newJob;

    EXPECT_CALL(*mockDbClient, execSqlAsync(_, _, _, _))
        .WillOnce(Invoke([&newJob](const std::string&, std::function<void(const drogon::orm::Result&)> callback, std::function<void(const std::exception_ptr&)>, const std::vector<std::string>&) {
            callback(drogon::orm::Result());
        }));

    bool called = false;
    auto callback = [&called](const HttpResponsePtr& resp) {
        called = true;
        EXPECT_EQ(resp->getStatusCode(), HttpStatusCode::k201Created);
        EXPECT_TRUE(resp->getJsonObject());
    };

    controller.createOne(req, std::move(callback), std::move(newJob));
    EXPECT_TRUE(called);
}

TEST_F(JobsControllerTest, CreateOneJob_DatabaseError) {
    auto req = HttpRequest::newHttpRequest();
    req->setPath("/jobs");
    req->setMethod(Post);
    Job newJob;

    EXPECT_CALL(*mockDbClient, execSqlAsync(_, _, _, _))
        .WillOnce(Invoke([](const std::string&, std::function<void(const drogon::orm::Result&)>, std::function<void(const std::exception_ptr&)> errorCallback, const std::vector<std::string>&) {
            errorCallback(std::make_exception_ptr(std::runtime_error("DB Error")));
        }));

    bool called = false;
    auto callback = [&called](const HttpResponsePtr& resp) {
        called = true;
        EXPECT_EQ(resp->getStatusCode(), HttpStatusCode::k500InternalServerError);
    };

    controller.createOne(req, std::move(callback), std::move(newJob));
    EXPECT_TRUE(called);
}

TEST_F(JobsControllerTest, UpdateOneJob_Success) {
    auto req = HttpRequest::newHttpRequest();
    req->setPath("/jobs/1");
    req->setMethod(Put);
    req->setJsonObject(std::make_shared<Json::Value>());
    Job updateJob;

    EXPECT_CALL(*mockDbClient, execSqlAsyncFuture(_, _))
        .WillOnce(Invoke([](const std::string&, const std::vector<std::string>&) {
            return std::shared_future<drogon::orm::Result>(drogon::orm::Result());
        }));
    EXPECT_CALL(*mockDbClient, execSqlAsync(_, _, _, _))
        .WillOnce(Invoke([](const std::string&, std::function<void(const drogon::orm::Result&)> callback, std::function<void(const std::exception_ptr&)>, const std::vector<std::string>&) {
            callback(drogon::orm::Result());
        }));

    bool called = false;
    auto callback = [&called](const HttpResponsePtr& resp) {
        called = true;
        EXPECT_EQ(resp->getStatusCode(), HttpStatusCode::k204NoContent);
    };

    controller.updateOne(req, std::move(callback), 1, std::move(updateJob));
    EXPECT_TRUE(called);
}

TEST_F(JobsControllerTest, UpdateOneJob_NotFound) {
    auto req = HttpRequest::newHttpRequest();
    req->setPath("/jobs/1");
    req->setMethod(Put);
    req->setJsonObject(std::make_shared<Json::Value>());
    Job updateJob;

    EXPECT_CALL(*mockDbClient, execSqlAsyncFuture(_, _))
        .WillOnce(Invoke([](const std::string&, const std::vector<std::string>&) {
            throw drogon::orm::UnexpectedRows("Not found");
        }));

    bool called = false;
    auto callback = [&called](const HttpResponsePtr& resp) {
        called = true;
        EXPECT_EQ(resp->getStatusCode(), HttpStatusCode::k404NotFound);
    };

    controller.updateOne(req, std::move(callback), 1, std::move(updateJob));
    EXPECT_TRUE(called);
}

TEST_F(JobsControllerTest, DeleteOneJob_Success) {
    auto req = HttpRequest::newHttpRequest();
    req->setPath("/jobs/1");
    req->setMethod(Delete);

    EXPECT_CALL(*mockDbClient, execSqlAsync(_, _, _, _))
        .WillOnce(Invoke([](const std::string&, std::function<void(const drogon::orm::Result&)> callback, std::function<void(const std::exception_ptr&)>, const std::vector<std::string>&) {
            callback(drogon::orm::Result());
        }));

    bool called = false;
    auto callback = [&called](const HttpResponsePtr& resp) {
        called = true;
        EXPECT_EQ(resp->getStatusCode(), HttpStatusCode::k204NoContent);
    };

    controller.deleteOne(req, std::move(callback), 1);
    EXPECT_TRUE(called);
}

TEST_F(JobsControllerTest, DeleteOneJob_DatabaseError) {
    auto req = HttpRequest::newHttpRequest();
    req->setPath("/jobs/1");
    req->setMethod(Delete);

    EXPECT_CALL(*mockDbClient, execSqlAsync(_, _, _, _))
        .WillOnce(Invoke([](const std::string&, std::function<void(const drogon::orm::Result&)>, std::function<void(const std::exception_ptr&)> errorCallback, const std::vector<std::string>&) {
            errorCallback(std::make_exception_ptr(std::runtime_error("DB Error")));
        }));

    bool called = false;
    auto callback = [&called](const HttpResponsePtr& resp) {
        called = true;
        EXPECT_EQ(resp->getStatusCode(), HttpStatusCode::k500InternalServerError);
    };

    controller.deleteOne(req, std::move(callback), 1);
    EXPECT_TRUE(called);
}

TEST_F(JobsControllerTest, GetJobPersons_Success) {
    auto req = HttpRequest::newHttpRequest();
    req->setPath("/jobs/1/persons");
    req->setMethod(Get);

    EXPECT_CALL(*mockDbClient, execSqlAsyncFuture(_, _))
        .WillOnce(Invoke([](const std::string&, const std::vector<std::string>&) {
            return std::shared_future<drogon::orm::Result>(drogon::orm::Result());
        }));
    EXPECT_CALL(*mockDbClient, execSqlAsync(_, _, _, _))
        .WillOnce(Invoke([](const std::string&, std::function<void(const drogon::orm::Result&)> callback, std::function<void(const std::exception_ptr&)>, const std::vector<std::string>&) {
            callback(drogon::orm::Result());
        }));

    bool called = false;
    auto callback = [&called](const HttpResponsePtr& resp) {
        called = true;
        EXPECT_EQ(resp->getStatusCode(), HttpStatusCode::k200OK);
    };

    controller.getJobPersons(req, std::move(callback), 1);
    EXPECT_TRUE(called);
}

TEST_F(JobsControllerTest, GetJobPersons_JobNotFound) {
    auto req = HttpRequest::newHttpRequest();
    req->setPath("/jobs/1/persons");
    req->setMethod(Get);

    EXPECT_CALL(*mockDbClient, execSqlAsyncFuture(_, _))
        .WillOnce(Invoke([](const std::string&, const std::vector<std::string>&) {
            throw drogon::orm::UnexpectedRows("Not found");
        }));

    bool called = false;
    auto callback = [&called](const HttpResponsePtr& resp) {
        called = true;
        EXPECT_EQ(resp->getStatusCode(), HttpStatusCode::k404NotFound);
    };

    controller.getJobPersons(req, std::move(callback), 1);
    EXPECT_TRUE(called);
}