include "gcli/sshkeys.h";

parser gitlab_sshkey is
object of gcli_sshkey with
	("title" => title as string,
	 "id" => id as int,
	 "key" => key as string,
	 "created_at" => created_at as string);

parser gitlab_sshkeys is array of gcli_sshkey use parse_gitlab_sshkey;
