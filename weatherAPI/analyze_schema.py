#!/usr/bin/env python3
import psycopg2

host = 'somwhere.no'
port = '5432'
dbname = 'the_weather_database'
user = 'nobody'
password = 'not_even_close'

conn = psycopg2.connect(host=host, port=port, database=dbname, user=user, password=password)
conn.set_session(autocommit=True)

cur = conn.cursor()
print('=== Database Schema Analysis ===\n')

# Base tables
base_tables = ['humidity', 'pressure', 'rain', 'temp', 'wind']
print('--- BASE TABLES ---')
for table in base_tables:
    cur.execute('''
        SELECT column_name, data_type
        FROM information_schema.columns
        WHERE table_name = %s
        ORDER BY ordinal_position
    ''', (table,))
    columns = cur.fetchall()
    print('\n{}:'.format(table.upper()))
    for col in columns:
        print('  - {}: {}'.format(col[0], col[1]))
    
    # Get sample
    try:
        cur.execute('SELECT * FROM {} ORDER BY time DESC LIMIT 1'.format(table))
        sample = cur.fetchone()
        col_names = [desc[0] for desc in cur.description]
        if sample:
            print('  Sample:')
            for col_name, value in zip(col_names, sample):
                print('    {}: {}'.format(col_name, value))
    except:
        pass

# Aggregate views
print('\n\n--- AGGREGATE VIEWS ---')
aggregates = ['_1d', '_1h', '_5m']
for base in base_tables:
    for agg in aggregates:
        view_name = base + agg
        cur.execute('''
            SELECT column_name, data_type
            FROM information_schema.columns
            WHERE table_name = %s
            ORDER BY ordinal_position
        ''', (view_name,))
        columns = cur.fetchall()
        if columns:
            print('\n{}:'.format(view_name.upper()))
            for col in columns:
                print('  - {}: {}'.format(col[0], col[1]))
            
            # Get sample
            try:
                cur.execute('SELECT * FROM {} ORDER BY time DESC LIMIT 1'.format(view_name))
                sample = cur.fetchone()
                col_names = [desc[0] for desc in cur.description]
                if sample:
                    print('  Sample:')
                    for col_name, value in zip(col_names, sample):
                        print('    {}: {}'.format(col_name, value))
            except:
                pass

cur.close()
conn.close()
