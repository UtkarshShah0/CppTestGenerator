// File: test_PersonsController.cpp
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <drogon/HttpController.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <drogon/app.h>
#include "../PersonsController.h"
#include "../models/Person.h"
#include "../models/PersonInfo.h"
#include "../utils/utils.h"

using namespace drogon;
using namespace drogon_model::org_chart;
using namespace testing;

class MockDbClient : public drogon::orm::DbClient {
public:
    MOCK_METHOD(void, execSqlAsync, (const std::string&, std::function<void(const drogon::orm::Result&)>, std::function<void(const drogon::orm::DrogonDbException&)>), (override));
    MOCK_METHOD(void, execSqlAsync, (const std::string&, const std::vector<std::string>&, std::function<void(const drogon::orm::Result&)>, std::function<void(const drogon::orm::DrogonDbException&)>), (override));
    MOCK_METHOD(std::shared_ptr<drogon::orm::Transaction>, newTransaction, (), (override));
    MOCK_METHOD(void, newTransactionAsync, (std::function<void(std::shared_ptr<drogon::orm::Transaction>)>, std::function<void(const drogon::orm::DrogonDbException&)>), (override));
    MOCK_METHOD(bool, hasAvailableConnections, (), (const, override));
    MOCK_METHOD(std::string, replaceSqlPlaceholders, (const std::string&, std::vector<std::string>&), (const, override));
};

class PersonsControllerTest : public ::testing::Test {
protected:
    void SetUp() override {
        mockDbClient = std::make_shared<MockDbClient>();
        drogon::app().registerDbClient(mockDbClient);
    }

    void TearDown() override {
        drogon::app().clearDbClients();
    }

    std::shared_ptr<MockDbClient> mockDbClient;
    PersonsController controller;
};

TEST_F(PersonsControllerTest, GetPersons_Success) {
    auto req = HttpRequest::newHttpRequest();
    req->setPath("/persons");
    req->setParameter("sort_field", "first_name");
    req->setParameter("sort_order", "desc");
    req->setParameter("limit", "10");
    req->setParameter("offset", "5");

    EXPECT_CALL(*mockDbClient, execSqlAsync(_, _, _))
        .WillOnce(Invoke([](const std::string& sql, auto, auto successCallback) {
            drogon::orm::Result result;
            result.push_back(drogon::orm::Row());
            successCallback(result);
        }));

    std::promise<HttpResponsePtr> responsePromise;
    auto callback = [&responsePromise](const HttpResponsePtr& resp) {
        responsePromise.set_value(resp);
    };

    controller.get(req, std::move(callback));
    auto resp = responsePromise.get_future().get();

    EXPECT_EQ(resp->getStatusCode(), HttpStatusCode::k200OK);
    EXPECT_TRUE(resp->getJsonObject() != nullptr);
}

TEST_F(PersonsControllerTest, GetPersons_EmptyResult) {
    auto req = HttpRequest::newHttpRequest();
    req->setPath("/persons");

    EXPECT_CALL(*mockDbClient, execSqlAsync(_, _, _))
        .WillOnce(Invoke([](const std::string& sql, auto, auto successCallback) {
            drogon::orm::Result result;
            successCallback(result);
        }));

    std::promise<HttpResponsePtr> responsePromise;
    auto callback = [&responsePromise](const HttpResponsePtr& resp) {
        responsePromise.set_value(resp);
    };

    controller.get(req, std::move(callback));
    auto resp = responsePromise.get_future().get();

    EXPECT_EQ(resp->getStatusCode(), HttpStatusCode::k404NotFound);
    EXPECT_TRUE(resp->getJsonObject() != nullptr);
    EXPECT_EQ(resp->getJsonObject()->get("error", "").asString(), "resource not found");
}

TEST_F(PersonsControllerTest, GetPersons_DbError) {
    auto req = HttpRequest::newHttpRequest();
    req->setPath("/persons");

    EXPECT_CALL(*mockDbClient, execSqlAsync(_, _, _))
        .WillOnce(Invoke([](const std::string& sql, auto, auto, auto errorCallback) {
            drogon::orm::DrogonDbException e("DB Error");
            errorCallback(e);
        }));

    std::promise<HttpResponsePtr> responsePromise;
    auto callback = [&responsePromise](const HttpResponsePtr& resp) {
        responsePromise.set_value(resp);
    };

    controller.get(req, std::move(callback));
    auto resp = responsePromise.get_future().get();

    EXPECT_EQ(resp->getStatusCode(), HttpStatusCode::k500InternalServerError);
    EXPECT_TRUE(resp->getJsonObject() != nullptr);
    EXPECT_EQ(resp->getJsonObject()->get("error", "").asString(), "database error");
}

TEST_F(PersonsControllerTest, GetOnePerson_Success) {
    auto req = HttpRequest::newHttpRequest();
    req->setPath("/persons/1");

    EXPECT_CALL(*mockDbClient, execSqlAsync(_, _, _))
        .WillOnce(Invoke([](const std::string& sql, auto, auto successCallback) {
            drogon::orm::Result result;
            result.push_back(drogon::orm::Row());
            successCallback(result);
        }));

    std::promise<HttpResponsePtr> responsePromise;
    auto callback = [&responsePromise](const HttpResponsePtr& resp) {
        responsePromise.set_value(resp);
    };

    controller.getOne(req, std::move(callback), 1);
    auto resp = responsePromise.get_future().get();

    EXPECT_EQ(resp->getStatusCode(), HttpStatusCode::k200OK);
    EXPECT_TRUE(resp->getJsonObject() != nullptr);
}

TEST_F(PersonsControllerTest, GetOnePerson_NotFound) {
    auto req = HttpRequest::newHttpRequest();
    req->setPath("/persons/1");

    EXPECT_CALL(*mockDbClient, execSqlAsync(_, _, _))
        .WillOnce(Invoke([](const std::string& sql, auto, auto successCallback) {
            drogon::orm::Result result;
            successCallback(result);
        }));

    std::promise<HttpResponsePtr> responsePromise;
    auto callback = [&responsePromise](const HttpResponsePtr& resp) {
        responsePromise.set_value(resp);
    };

    controller.getOne(req, std::move(callback), 1);
    auto resp = responsePromise.get_future().get();

    EXPECT_EQ(resp->getStatusCode(), HttpStatusCode::k404NotFound);
    EXPECT_TRUE(resp->getJsonObject() != nullptr);
    EXPECT_EQ(resp->getJsonObject()->get("error", "").asString(), "resource not found");
}

TEST_F(PersonsControllerTest, CreateOnePerson_Success) {
    auto req = HttpRequest::newHttpRequest();
    req->setPath("/persons");
    req->setMethod(Post);
    Json::Value json;
    json["first_name"] = "John";
    json["last_name"] = "Doe";
    json["department_id"] = "1";
    json["job_id"] = "1";
    json["manager_id"] = "2";
    req->setJsonObject(json);

    EXPECT_CALL(*mockDbClient, execSqlAsync(_, _, _))
        .WillOnce(Invoke([](const std::string& sql, auto, auto successCallback) {
            drogon::orm::Result result;
            result.push_back(drogon::orm::Row());
            successCallback(result);
        }));

    std::promise<HttpResponsePtr> responsePromise;
    auto callback = [&responsePromise](const HttpResponsePtr& resp) {
        responsePromise.set_value(resp);
    };

    Person person;
    controller.createOne(req, std::move(callback), std::move(person));
    auto resp = responsePromise.get_future().get();

    EXPECT_EQ(resp->getStatusCode(), HttpStatusCode::k201Created);
    EXPECT_TRUE(resp->getJsonObject() != nullptr);
}

TEST_F(PersonsControllerTest, UpdateOnePerson_Success) {
    auto req = HttpRequest::newHttpRequest();
    req->setPath("/persons/1");
    req->setMethod(Put);
    Json::Value json;
    json["first_name"] = "Jane";
    req->setJsonObject(json);

    EXPECT_CALL(*mockDbClient, execSqlAsync(_, _, _))
        .Times(2)
        .WillRepeatedly(Invoke([](const std::string& sql, auto, auto successCallback) {
            drogon::orm::Result result;
            result.push_back(drogon::orm::Row());
            successCallback(result);
        }));

    std::promise<HttpResponsePtr> responsePromise;
    auto callback = [&responsePromise](const HttpResponsePtr& resp) {
        responsePromise.set_value(resp);
    };

    Person person;
    controller.updateOne(req, std::move(callback), 1, std::move(person));
    auto resp = responsePromise.get_future().get();

    EXPECT_EQ(resp->getStatusCode(), HttpStatusCode::k204NoContent);
}

TEST_F(PersonsControllerTest, UpdateOnePerson_NotFound) {
    auto req = HttpRequest::newHttpRequest();
    req->setPath("/persons/1");
    req->setMethod(Put);

    EXPECT_CALL(*mockDbClient, execSqlAsync(_, _, _))
        .WillOnce(Invoke([](const std::string& sql, auto, auto, auto errorCallback) {
            drogon::orm::DrogonDbException e("Not Found");
            errorCallback(e);
        }));

    std::promise<HttpResponsePtr> responsePromise;
    auto callback = [&responsePromise](const HttpResponsePtr& resp) {
        responsePromise.set_value(resp);
    };

    Person person;
    controller.updateOne(req, std::move(callback), 1, std::move(person));
    auto resp = responsePromise.get_future().get();

    EXPECT_EQ(resp->getStatusCode(), HttpStatusCode::k404NotFound);
    EXPECT_TRUE(resp->getJsonObject() != nullptr);
    EXPECT_EQ(resp->getJsonObject()->get("error", "").asString(), "resource not found");
}

TEST_F(PersonsControllerTest, DeleteOnePerson_Success) {
    auto req = HttpRequest::newHttpRequest();
    req->setPath("/persons/1");
    req->setMethod(Delete);

    EXPECT_CALL(*mockDbClient, execSqlAsync(_, _, _))
        .WillOnce(Invoke([](const std::string& sql, auto, auto successCallback) {
            drogon::orm::Result result;
            successCallback(result);
        }));

    std::promise<HttpResponsePtr> responsePromise;
    auto callback = [&responsePromise](const HttpResponsePtr& resp) {
        responsePromise.set_value(resp);
    };

    controller.deleteOne(req, std::move(callback), 1);
    auto resp = responsePromise.get_future().get();

    EXPECT_EQ(resp->getStatusCode(), HttpStatusCode::k204NoContent);
}

TEST_F(PersonsControllerTest, GetDirectReports_Success) {
    auto req = HttpRequest::newHttpRequest();
    req->setPath("/persons/1/reports");

    EXPECT_CALL(*mockDbClient, execSqlAsync(_, _, _))
        .Times(2)
        .WillRepeatedly(Invoke([](const std::string& sql, auto, auto successCallback) {
            drogon::orm::Result result;
            result.push_back(drogon::orm::Row());
            successCallback(result);
        }));

    std::promise<HttpResponsePtr> responsePromise;
    auto callback = [&responsePromise](const HttpResponsePtr& resp) {
        responsePromise.set_value(resp);
    };

    controller.getDirectReports(req, std::move(callback), 1);
    auto resp = responsePromise.get_future().get();

    EXPECT_EQ(resp->getStatusCode(), HttpStatusCode::k200OK);
    EXPECT_TRUE(resp->getJsonObject() != nullptr);
}

TEST_F(PersonsControllerTest, GetDirectReports_NotFound) {
    auto req = HttpRequest::newHttpRequest();
    req->setPath("/persons/1/reports");

    EXPECT_CALL(*mockDbClient, execSqlAsync(_, _, _))
        .WillOnce(Invoke([](const std::string& sql, auto, auto, auto errorCallback) {
            drogon::orm::DrogonDbException e("Not Found");
            errorCallback(e);
        }));

    std::promise<HttpResponsePtr> responsePromise;
    auto callback = [&responsePromise](const HttpResponsePtr& resp) {
        responsePromise.set_value(resp);
    };

    controller.getDirectReports(req, std::move(callback), 1);
    auto resp = responsePromise.get_future().get();

    EXPECT_EQ(resp->getStatusCode(), HttpStatusCode::k404NotFound);
    EXPECT_TRUE(resp->getJsonObject() != nullptr);
    EXPECT_EQ(resp->getJsonObject()->get("error", "").asString(), "resource not found");
}