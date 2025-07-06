// File: test_AuthController.cpp
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <drogon/app.h>
#include <drogon/orm/Mapper.h>
#include <third_party/libbcrypt/include/bcrypt/BCrypt.hpp>
#include "../AuthController.h"
#include "../plugins/JwtPlugin.h"
#include "../models/User.h"

using namespace drogon;
using namespace drogon::orm;
using namespace drogon_model::org_chart;
using ::testing::_;
using ::testing::Return;
using ::testing::NiceMock;

class MockJwtPlugin : public JwtPlugin {
public:
    MOCK_METHOD0(init, JwtPlugin::Jwt&());
    MOCK_METHOD2(encode, std::string(const std::string&, const std::string&));
};

class MockDbClient : public drogon::orm::DbClient {
public:
    MOCK_METHOD1(execSqlAsyncFuture, std::future<drogon::orm::Result>(const std::string&));
    MOCK_METHOD2(execSqlAsyncFuture, std::future<drogon::orm::Result>(const std::string&, const std::vector<std::string>&));
    MOCK_METHOD3(execSqlAsyncFuture, std::future<drogon::orm::Result>(const std::string&, const std::vector<std::string>&, const std::vector<const char*>&));
    MOCK_METHOD4(execSqlAsyncFuture, std::future<drogon::orm::Result>(const std::string&, const std::vector<std::string>&, const std::vector<const char*>&, const std::vector<size_t>&));
};

class AuthControllerTest : public ::testing::Test {
protected:
    void SetUp() override {
        drogon::app().registerPlugin(&mockJwtPlugin);
        ON_CALL(mockJwtPlugin, encode(_, _)).WillByDefault(Return("mocked_token"));
    }

    void TearDown() override {
        drogon::app().getPlugin<MockJwtPlugin>()->shutdown();
    }

    AuthController controller;
    NiceMock<MockJwtPlugin> mockJwtPlugin;
};

TEST_F(AuthControllerTest, RegisterUser_MissingFields_ReturnsBadRequest) {
    User user;
    auto req = HttpRequest::newHttpRequest();
    HttpResponsePtr response;
    auto callback = [&response](const HttpResponsePtr& resp) { response = resp; };

    controller.registerUser(req, std::move(callback), std::move(user));

    ASSERT_EQ(response->getStatusCode(), HttpStatusCode::k400BadRequest);
    auto json = response->getJsonObject();
    EXPECT_EQ((*json)["error"].asString(), "missing fields");
}

TEST_F(AuthControllerTest, RegisterUser_UsernameTaken_ReturnsBadRequest) {
    User user;
    user.setUsername("testuser");
    user.setPassword("password");
    auto req = HttpRequest::newHttpRequest();
    HttpResponsePtr response;
    auto callback = [&response](const HttpResponsePtr& resp) { response = resp; };

    controller.registerUser(req, std::move(callback), std::move(user));

    ASSERT_EQ(response->getStatusCode(), HttpStatusCode::k400BadRequest);
    auto json = response->getJsonObject();
    EXPECT_EQ((*json)["error"].asString(), "username is taken");
}

TEST_F(AuthControllerTest, RegisterUser_ValidInput_ReturnsCreated) {
    User user;
    user.setUsername("newuser");
    user.setPassword("password");
    auto req = HttpRequest::newHttpRequest();
    HttpResponsePtr response;
    auto callback = [&response](const HttpResponsePtr& resp) { response = resp; };

    controller.registerUser(req, std::move(callback), std::move(user));

    ASSERT_EQ(response->getStatusCode(), HttpStatusCode::k201Created);
    auto json = response->getJsonObject();
    EXPECT_EQ((*json)["username"].asString(), "newuser");
    EXPECT_FALSE((*json)["token"].asString().empty());
}

TEST_F(AuthControllerTest, LoginUser_MissingFields_ReturnsBadRequest) {
    User user;
    auto req = HttpRequest::newHttpRequest();
    HttpResponsePtr response;
    auto callback = [&response](const HttpResponsePtr& resp) { response = resp; };

    controller.loginUser(req, std::move(callback), std::move(user));

    ASSERT_EQ(response->getStatusCode(), HttpStatusCode::k400BadRequest);
    auto json = response->getJsonObject();
    EXPECT_EQ((*json)["error"].asString(), "missing fields");
}

TEST_F(AuthControllerTest, LoginUser_UserNotFound_ReturnsBadRequest) {
    User user;
    user.setUsername("unknown");
    user.setPassword("password");
    auto req = HttpRequest::newHttpRequest();
    HttpResponsePtr response;
    auto callback = [&response](const HttpResponsePtr& resp) { response = resp; };

    controller.loginUser(req, std::move(callback), std::move(user));

    ASSERT_EQ(response->getStatusCode(), HttpStatusCode::k400BadRequest);
    auto json = response->getJsonObject();
    EXPECT_EQ((*json)["error"].asString(), "user not found");
}

TEST_F(AuthControllerTest, LoginUser_InvalidPassword_ReturnsUnauthorized) {
    User user;
    user.setUsername("testuser");
    user.setPassword("wrongpassword");
    auto req = HttpRequest::newHttpRequest();
    HttpResponsePtr response;
    auto callback = [&response](const HttpResponsePtr& resp) { response = resp; };

    controller.loginUser(req, std::move(callback), std::move(user));

    ASSERT_EQ(response->getStatusCode(), HttpStatusCode::k401Unauthorized);
    auto json = response->getJsonObject();
    EXPECT_EQ((*json)["error"].asString(), "username and password do not match");
}

TEST_F(AuthControllerTest, LoginUser_ValidCredentials_ReturnsOk) {
    User user;
    user.setUsername("testuser");
    user.setPassword("correctpassword");
    auto req = HttpRequest::newHttpRequest();
    HttpResponsePtr response;
    auto callback = [&response](const HttpResponsePtr& resp) { response = resp; };

    controller.loginUser(req, std::move(callback), std::move(user));

    ASSERT_EQ(response->getStatusCode(), HttpStatusCode::k200OK);
    auto json = response->getJsonObject();
    EXPECT_EQ((*json)["username"].asString(), "testuser");
    EXPECT_FALSE((*json)["token"].asString().empty());
}

TEST_F(AuthControllerTest, AreFieldsValid_ValidUser_ReturnsTrue) {
    User user;
    user.setUsername("testuser");
    user.setPassword("password");

    EXPECT_TRUE(controller.areFieldsValid(user));
}

TEST_F(AuthControllerTest, AreFieldsValid_MissingUsername_ReturnsFalse) {
    User user;
    user.setPassword("password");

    EXPECT_FALSE(controller.areFieldsValid(user));
}

TEST_F(AuthControllerTest, AreFieldsValid_MissingPassword_ReturnsFalse) {
    User user;
    user.setUsername("testuser");

    EXPECT_FALSE(controller.areFieldsValid(user));
}

TEST_F(AuthControllerTest, IsPasswordValid_ValidPassword_ReturnsTrue) {
    std::string password = "testpass";
    std::string hash = BCrypt::generateHash(password);

    EXPECT_TRUE(controller.isPasswordValid(password, hash));
}

TEST_F(AuthControllerTest, IsPasswordValid_InvalidPassword_ReturnsFalse) {
    std::string password = "testpass";
    std::string hash = BCrypt::generateHash("wrongpass");

    EXPECT_FALSE(controller.isPasswordValid(password, hash));
}

TEST_F(AuthControllerTest, UserWithToken_ConstructionAndToJson_ReturnsCorrectJson) {
    User user;
    user.setId(1);
    user.setUsername("testuser");
    AuthController::UserWithToken userWithToken(user);

    auto json = userWithToken.toJson();
    EXPECT_EQ(json["username"].asString(), "testuser");
    EXPECT_EQ(json["token"].asString(), "mocked_token");
}