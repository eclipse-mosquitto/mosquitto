#include "config.h"

#include <stdio.h>
#include <CUnit/CUnit.h>
#include "mosquitto.h"

#ifdef WITH_TLS

static void check_sha512(const char *encoded, const char *password)
{
	struct mosquitto_pw *pw = NULL;

	CU_ASSERT_EQUAL_FATAL(mosquitto_pw_new(&pw, MOSQ_PW_SHA512), MOSQ_ERR_SUCCESS);
	CU_ASSERT_EQUAL(mosquitto_pw_decode(pw, encoded), MOSQ_ERR_SUCCESS);
	CU_ASSERT_EQUAL(mosquitto_pw_verify(pw, password), MOSQ_ERR_SUCCESS);
	CU_ASSERT_EQUAL(mosquitto_pw_verify(pw, "wrong-password"), MOSQ_ERR_AUTH);
	mosquitto_pw_cleanup(pw);
}

/* Independent SHA512(password || salt) vectors, with salts bytes(range(n)). */

static void TEST_sha512_empty(void)
{
	check_sha512(
		"$6$AAECAwQFBgcICQoL$ulGyqdovJv6B/D7hFSQlWTfsa+xIg160N8WYxVZ04VqlD4iSLedYQzKl5NJHhwkMsU38Or2znFWu327hCPlTVA==",
		"");
}

static void TEST_sha512_one_byte(void)
{
	check_sha512(
		"$6$AAECAwQFBgcICQoL$jia8nwnr4sv+8A34tpRB6F3i2bG5SE81vzqrS6AVDVJamk3hzqpYX57G0VrMR0CHTdbv0QOU1feXAjUQ14gxHA==",
		"a");
}

static void TEST_sha512_two_bytes(void)
{
	check_sha512(
		"$6$AAECAwQFBgcICQoL$XPCUsViEkYxhiR/QjQMkpsUPHyKJJAi6qF2akqGrpbyK/qJV9nH2B+kkW28Pvsk+d108JvdScsTS8beoM75bWQ==",
		"ab");
}

static void TEST_sha512_legacy_salt(void)
{
	check_sha512(
		"$6$AAECAwQFBgcICQoL$Xwdqrs/bNYAEaPVTFplq7USgn/Nev3kbVxnW666xNP0VdEHj7Oa6BtetwUSYWe6CRmMk6EENDwzV/4tYiCNrJw==",
		"test-password");
}

static void TEST_sha512_current_salt(void)
{
	check_sha512(
		"$6$AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4OTo7PD0+Pw==$QZWbiDpzXZ6JeRHxtP7pZbynt4HPgkd76HU0Kj5YmrYNkh3znBeLoB9ApU3HKQyj8VTdIFJgWXb0gkIJLFoBBQ==",
		"test-password");
}

static void TEST_sha512_generated_password(void)
{
	struct mosquitto_pw *pw = NULL;

	CU_ASSERT_EQUAL_FATAL(mosquitto_pw_new(&pw, MOSQ_PW_SHA512), MOSQ_ERR_SUCCESS);
	CU_ASSERT_EQUAL(mosquitto_pw_hash_encoded(pw, "test-password"), MOSQ_ERR_SUCCESS);
	CU_ASSERT_EQUAL(mosquitto_pw_verify(pw, "test-password"), MOSQ_ERR_SUCCESS);
	CU_ASSERT_EQUAL(mosquitto_pw_verify(pw, "wrong-password"), MOSQ_ERR_AUTH);
	mosquitto_pw_cleanup(pw);
}

int init_password_tests(void)
{
	CU_pSuite suite = CU_add_suite("password", NULL, NULL);
	if(!suite){
		return 1;
	}

	if(0
			|| !CU_add_test(suite, "SHA512 empty", TEST_sha512_empty)
			|| !CU_add_test(suite, "SHA512 one byte", TEST_sha512_one_byte)
			|| !CU_add_test(suite, "SHA512 two bytes", TEST_sha512_two_bytes)
			|| !CU_add_test(suite, "SHA512 legacy salt", TEST_sha512_legacy_salt)
			|| !CU_add_test(suite, "SHA512 current salt", TEST_sha512_current_salt)
			|| !CU_add_test(suite, "SHA512 generated password", TEST_sha512_generated_password)
			){
		return 1;
	}
	return 0;
}
#endif
