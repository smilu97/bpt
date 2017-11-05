
f = open('test_input/test.txt', 'w')

res = ''
for i in range(1000000):
    res += 'i {} a{}\n'.format(i, i)

f.write(res)

f.close()