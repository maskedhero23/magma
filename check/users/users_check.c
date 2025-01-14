
/**
 * @file /check/users/users_check.c
 *
 * @brief Checks the code used to handle user data.
 *
 * $Author$
 * $Date$
 * $Revision$
 *
 */

#include "magma_check.h"

START_TEST (check_users_credentials_valid_s) {

	int_t state, cred_res;
	salt_state_t salt_res;
	stringer_t *errmsg = NULL, *salt = NULL;
	meta_user_t *user_check_data = NULL;
	credential_t *user_check_cred = NULL;


	typedef struct {
		stringer_t *username;
		stringer_t *password;
	} name_pass;
#if 0

/** use this to generate new user info*/
	stringer_t *temp_salt, *temp_hex;
	credential_t *stacie;

	fprintf(stderr, "\n\n");

	stacie = credential_alloc_auth(CONSTANT("stacie"));

	temp_salt = credential_salt_generate();
	temp_hex = hex_encode_opts(temp_salt, (MANAGED_T | CONTIGUOUS | HEAP));

	fprintf(stderr, "%s\n", st_char_get(temp_hex));

	credential_calc_auth(stacie, CONSTANT("magma"), temp_salt);

	fprintf(stderr, "%s\n\n", st_char_get(stacie->auth.password));
#endif
	name_pass tests[4] = {
		{
			CONSTANT("magma"),
			CONSTANT("test")
		},
		{
			CONSTANT("magma@lavabit.com"),
			CONSTANT("test")
		},
		{
			CONSTANT("magma+label@lavabit.com"),
			CONSTANT("test")
		},
		{
			CONSTANT("stacie"),
			CONSTANT("magma")
		}
	};

	// Valid Login Attempts
	log_unit("%-64.64s", "USERS / CREDENTIAL / VALID / SINGLE THREADED:");

	for(uint_t i = 0; i < (sizeof(tests)/sizeof(tests[0])); ++i) {

		if(!(user_check_cred = credential_alloc_auth(tests[i].username))) {
			errmsg = st_aprint("Credential allocation failed. { user = %s }", st_char_get(tests[i].username));
			goto error;
		}

		salt_res = credential_salt_fetch(user_check_cred->auth.username, &salt);

		if(salt_res == USER_SALT) {
			cred_res = credential_calc_auth(user_check_cred, tests[i].password, salt);
			st_free(salt);
		}
		else if(salt_res == USER_NO_SALT) {
			cred_res = credential_calc_auth(user_check_cred, tests[i].password, NULL);
		}
		else {
			errmsg = st_aprint("Error looking for user salt. { user = %s }", st_char_get(tests[i].username));
			goto error;
		}

		if(!cred_res) {
			errmsg = st_aprint("Credential allocation failed. { password = %s / salt = NULL }", st_char_get(tests[i].password));
			goto cleanup_cred;
		}

		state = meta_get(user_check_cred, META_PROT_GENERIC, (META_GET_MESSAGES | META_GET_FOLDERS | META_GET_CONTACTS), &user_check_data);

		if(state != 1) {
			errmsg = st_aprint("Authentication failed. { user = %s / password = %s / salt = NULL / meta_get = %i }",
					st_char_get(tests[i].username), st_char_get(tests[i].password), state);
			goto cleanup_cred;
		}

		meta_remove(user_check_cred->auth.username, META_PROT_GENERIC);

		if(meta_user_prune(user_check_cred->auth.username) < 1) {
			errmsg = st_aprint("An error occurred while trying to prune the user data from the object cache. { user = %s }", st_char_get(tests[i].username));
			goto cleanup_cred;
		}

		credential_free(user_check_cred);
	}

	log_unit("%10.10s\n", (!status() ? "SKIPPED" : !errmsg ? "PASSED" : "FAILED"));
	fail_unless(!errmsg, st_char_get(errmsg));
	st_cleanup(errmsg);
	return;

cleanup_cred:
	credential_free(user_check_cred);
error:
	log_unit("%10.10s\n", (!status() ? "SKIPPED" : !errmsg ? "PASSED" : "FAILED"));
	fail_unless(!errmsg, st_char_get(errmsg));
	st_cleanup(errmsg);

} END_TEST

START_TEST (check_users_credentials_invalid_s) {

	int_t state;
	stringer_t *errmsg = NULL;
	meta_user_t *user_check_data = NULL;
	credential_t *user_check_cred = NULL;

	typedef struct {
		stringer_t *username;
		stringer_t *password;
	} name_pass;

	name_pass tests[4] = {
		{
			CONSTANT("magma"),
			CONSTANT("password")
		},
		{
			CONSTANT("magma@lavabit.com"),
			CONSTANT("password")
		},
		{
			CONSTANT("magma+label@lavabit.com"),
			CONSTANT("password")
		},
		{
			CONSTANT("magma@nerdshack.com"),
			CONSTANT("test")
		}
	};

	// Invalid Login Attempts
	log_unit("%-64.64s", "USERS / CREDENTIAL / INVALID / SINGLE THREADED:");

	// Try passing in various combinations of NULL.
	if ((user_check_cred = credential_alloc_auth(NULL))) {
		errmsg = st_merge("n", "Credential allocation should have failed but succeeded instead. { user = NULL }");
		goto cleanup_cred;
	}

	if (!(user_check_cred = credential_alloc_auth(CONSTANT("magma")))) {
		errmsg = st_merge("n", "Credential allocation failed. { user = magma }");
		goto error;
	}
	else {
		credential_free(user_check_cred);
	}

	for(uint_t i = 0; i < sizeof(tests)/sizeof(tests[0]); ++i) {

		if (!(user_check_cred = credential_alloc_auth(tests[i].username))) {
			errmsg = st_aprint("Credential creation failed. Authentication was not attempted. { user = %s }", st_char_get(tests[i].username));
			goto error;
		}

		if(!credential_calc_auth(user_check_cred, tests[i].password, NULL)) {
			errmsg = st_aprint("Credential calculation failed. { password = %s / salt = NULL}", st_char_get(tests[i].password));
			goto cleanup_cred;
		}

		if ((state = meta_get(user_check_cred, META_PROT_GENERIC, META_GET_MESSAGES | META_GET_FOLDERS | META_GET_CONTACTS, &(user_check_data))) == 1) {
			errmsg = st_aprint("Authentication should have failed but succeeded instead. { user = %s / password = %s / meta_get = %i }",
					st_char_get(tests[i].username), st_char_get(tests[i].password), state);
			goto cleanup_data;
		}

		credential_free(user_check_cred);
	}

	log_unit("%10.10s\n", (!status() ? "SKIPPED" : !errmsg ? "PASSED" : "FAILED"));
	fail_unless(!errmsg, st_char_get(errmsg));
	return;

cleanup_data:
	meta_remove(user_check_cred->auth.username, META_PROT_GENERIC);
cleanup_cred:
	credential_free(user_check_cred);
error:
	log_unit("%10.10s\n", (!status() ? "SKIPPED" : !errmsg ? "PASSED" : "FAILED"));
	fail_unless(!errmsg, st_char_get(errmsg));

	// Attempt a credentials creation using a series of randomly generated, but valid usernames.
	/* In my opinion these tests are no good so I am commenting them out - IVAN */
#if 0
	stringer_t *username = NULL, *password = NULL;
	for (uint_t i = 0; !errmsg && i < OBJECT_CHECK_ITERATIONS; i++) {
		if (!(username = rand_choices("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_", (rand_get_uint8() % 128) + 1))) {
			errmsg = st_import("An error occurred while trying to generate a random username. { user = NULL }", 78);
		}
		else if (!(user_check_cred = credential_alloc_auth(username, CONSTANT("test")))) {
			errmsg = st_aprint("Credential creation failed. Authentication was not attempted. { user = %.*s / password = test }",	st_length_int(username), st_char_get(username));
		}
		else if ((state = meta_get(user_check_cred, META_PROT_GENERIC, META_GET_MESSAGES | META_GET_FOLDERS | META_GET_CONTACTS, &(user_check_data))) != 0) {
			errmsg = st_aprint("Authentication should have failed but succeeded instead. { user = RANDOM BINARY DATA / password = RANDOM BINARY DATA / meta_get = %i }", state);
		}

		if (user_check_data) {
			meta_remove(user_check_cred->auth.username, META_PROT_GENERIC);
			user_check_data = NULL;
		}

		if (user_check_cred) {
			credential_free(user_check_cred);
			user_check_cred = NULL;
		}

		st_cleanup(username);
	}

	// Attempt a credentials creation using a series of randomly generated binary usernames.
	for (uint_t i = 0; !errmsg && i < OBJECT_CHECK_ITERATIONS; i++) {

		username = st_alloc((rand_get_uint8() % 128) + 1);

		if (!username || !rand_write(username)) {
			errmsg = st_import("An error occurred while trying to generate a random username. { user = NULL }", 78);
		}
		else if ((user_check_cred = credential_alloc_auth(username, CONSTANT("test"))) && (state = meta_get(user_check_cred,
				META_PROT_GENERIC, META_GET_MESSAGES | META_GET_FOLDERS | META_GET_CONTACTS, &(user_check_data))) != 0) {
			errmsg = st_aprint("Authentication should have failed but succeeded instead. { user = RANDOM BINARY DATA / password = test / meta_get = %i }", state);
		}

		if (user_check_data) {
			meta_remove(user_check_cred->auth.username, META_PROT_GENERIC);
			user_check_data = NULL;
		}

		if (user_check_cred) {
			credential_free(user_check_cred);
			user_check_cred = NULL;
		}

		st_cleanup(username);
	}

	// Finally, try generating credentials using randomly generated binary data for the username and the password.
	for (uint_t i = 0; !errmsg && i < OBJECT_CHECK_ITERATIONS; i++) {

		username = st_alloc((rand_get_uint8() % 128) + 1);
		password = st_alloc((rand_get_uint8() % 255) + 1);

		if (!username || !rand_write(username) || !password || !rand_write(password)) {
			errmsg = st_import("Random username and password generation failed. { user = NULL / password = NULL }", 82);
		}
		else if ((user_check_cred = credential_alloc_auth(username, password)) && (state = meta_get(user_check_cred, META_PROT_GENERIC, META_GET_MESSAGES | META_GET_FOLDERS | META_GET_CONTACTS, &(user_check_data))) != 0) {
			errmsg = st_aprint("Authentication should have failed but succeeded instead. { user = RANDOM BINARY DATA / password = RANDOM BINARY DATA / meta_get = %i }", state);
		}

		if (user_check_data) {
			meta_remove(user_check_cred->auth.username, META_PROT_GENERIC);
			user_check_data = NULL;
		}

		if (user_check_cred) {
			credential_free(user_check_cred);
			user_check_cred = NULL;
		}

		st_cleanup(username);
		st_cleanup(password);
	}
#endif


} END_TEST

START_TEST (check_users_inbox_s) {

	char *errmsg = NULL;

	log_unit("%-64.64s", "USERS / INBOX / SINGLE THREADED:");
	errmsg = "User inbox test incomplete.";
	log_unit("%10.10s\n", "SKIPPED");

	//log_unit("%10.10s\n", (!status() ? "SKIPPED" : !errmsg ? "PASSED" : "FAILED"));
	//fail_unless(!errmsg, errmsg);
} END_TEST

START_TEST (check_users_message_s) {

	char *errmsg = NULL;

	log_unit("%-64.64s", "USERS / MESSAGE / SINGLE THREADED:");
	errmsg = "User message test incomplete.";
	log_unit("%10.10s\n", "SKIPPED");

	//log_unit("%10.10s\n", (!status() ? "SKIPPED" : !errmsg ? "PASSED" : "FAILED"));
	//fail_unless(!errmsg, errmsg);
} END_TEST

Suite * suite_check_users(void) {

	TCase *tc;
	Suite *s = suite_create("\tUsers");

	testcase(s, tc, "Auth Valid/S", check_users_credentials_valid_s);
	testcase(s, tc, "Auth Invalid/S", check_users_credentials_invalid_s);
	testcase(s, tc, "Inbox/S", check_users_inbox_s);
	testcase(s, tc, "Message/S", check_users_message_s);

	return s;
}
