
drop database if exists pokemon;
create database pokemon;
use pokemon;

create table City (
	name varchar(256) primary key,
	description text
);

create table Pokemon (
	id int primary key auto_increment,
	name varchar(256),
	type varchar(128)
);

create table Trainer (
	id int primary key auto_increment,
	name varchar(256),
	hometown varchar(256),
	foreign key (hometown) references City(name)
);

create table CatchedPokemon (
	id int primary key auto_increment,
	owner_id int,
	pid int,
	level int,
	nickname varchar(256),
	foreign key (owner_id) references Trainer(id),
	foreign key (pid) references Pokemon(id)
);

create table Gym (
	leader_id int,
	city varchar(256),
	foreign key (leader_id) references Trainer(id),
	foreign key (city) references City(name)
);

create table Evolution (
	before_id int,
	after_id int,
	foreign key (before_id) references Pokemon(id),
	foreign key (after_id) references Pokemon(id)
);