-- 1
select name from Trainer as T where T.hometown = 'Blue City';

-- 2
select name from Trainer as T where T.hometown in ('Brown City', 'Rainbow City');

-- 3
select name, hometown from Trainer as T where T.name regexp '^[aeiouAEIOU]';

-- 4
select name from Pokemon as P where P.type = 'Water';

-- 5
select type from Pokemon group by type;

-- 6
select name from Pokemon as P order by P.name;

-- 7
select name from Pokemon where name like '%s';

-- 8
select name from Pokemon where name like '%e%s';

-- 9
select name from Pokemon where name regexp '^[aeiouAEIOU]';

-- 10
select type, count(*) from Pokemon group by type;

-- 11
select nickname from CatchedPokemon order by level desc limit 3;

-- 12
mysql> select avg(level) from CatchedPokemon;

-- 13
mysql> select max(level) - min(level) from CatchedPokemon;

-- 14
mysql> select count(*) from Pokemon where name regexp '^[bcde]';

-- 15
mysql> select count(*) from Pokemon where type not in ('Fire', 'Grass', 'Water', 'Electric');

-- 16
select T.name, P.name, C.nickname
from Trainer as T, Pokemon as P, CatchedPokemon as C
where C.nickname like '% %'
and C.owner_id = T.id
and C.pid = P.id;

-- 17
select T.name from Trainer as T, Pokemon as P, CatchedPokemon as C
where P.type = 'Psychic'
and C.owner_id = T.id
and C.pid = P.id;

-- 18
select T.name, T.hometown from Trainer as T
order by (
	select avg(level) from CatchedPokemon as C where C.owner_id = T.id
) desc;

-- 19
select T.name, count(*) from Trainer as T, CatchedPokemon as C
where C.owner_id = T.id
group by C.owner_id;

-- 20
select P.name, C.level
from Gym as G, Trainer as T, CatchedPokemon as C, Pokemon as P
where G.city = 'Sangnok City'
and G.leader_id = T.id
and C.owner_id = T.id
and C.pid = P.id;

-- 21
select P.name, count(*) from Pokemon as P, CatchedPokemon as C
where C.pid = P.id
group by C.pid
order by count(*) desc;

-- 22
select P1.name from Pokemon as P1
where P1.id = (
	select E1.after_id
	from Evolution as E1
	where E1.before_id = (
		select E2.after_id
		from Evolution as E2
		where E2.before_id = (
			select P2.id from Pokemon as P2
			where P2.name = 'Charmander'
		)
	)
);

-- 23
select P.name, count(*)
from Pokemon as P, CatchedPokemon as C
where P.id <= 30
and C.pid = P.id
group by C.pid
order by P.name;

-- 24
select T.name, min(P.type)
from Trainer as T
join CatchedPokemon as C on C.owner_id = T.id
join Pokemon as P on P.id = C.pid
group by T.id
having count(distinct P.type) = 1;

-- 25
select T.name, P.type, count(*)
from Trainer as T
join CatchedPokemon as C on C.owner_id = T.id
join Pokemon as P on P.id = C.pid
group by T.id, P.type

-- 26
select T.name, min(P.name), count(*)
from Trainer as T
join CatchedPokemon as C on C.owner_id = T.id
join Pokemon as P on P.id = C.pid
group by T.id
having count(distinct P.id) = 1;

-- 27
select T.name, min(G.city)
from Gym as G
join Trainer as T on G.leader_id = T.id
join CatchedPokemon as C on T.id = C.owner_id
join Pokemon as P on C.pid = P.id
group by T.id
having count(distinct P.type) > 1;

-- 28
select T.name, sum(C.level)
from Gym as G
join Trainer as T on T.id = G.leader_id
join CatchedPokemon as C on C.owner_id = T.id
where C.level >= 50
group by T.id;

-- 29
select P.name
from Pokemon as P
where exists(
	select * from Trainer as T
	join CatchedPokemon as C on C.owner_id = T.id
	join Pokemon as P2 on P2.id = C.pid
	where T.hometown = 'Sangnok City' and P2.id = P.id
) and exists(
	select * from Trainer as T
	join CatchedPokemon as C on C.owner_id = T.id
	join Pokemon as P2 on P2.id = C.pid
	where T.hometown = 'Blue City' and P2.id = P.id
);

-- 30
select P.name
from Pokemon as P
where not exists(
	select * from Evolution as E
	where E.before_id = P.id
) and exists(
	select * from Evolution as E
	where E.after_id = P.id
);