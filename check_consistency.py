import argparse
import sqlite3

def check_consistency(database_name, table_name, result_column):
    conn = sqlite3.connect(database_name)
    cur = conn.cursor()

    previous_results = cur.execute(f'SELECT {result_column} FROM {table_name}').fetchall()

    conn.close()

    if not previous_results:
        return True  # No previous results to compare

    # Check if all previous results are equal to the current result
    return all(r[0] == previous_results[0][0] for r in previous_results)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--database', required=True,
                        help='The name of the database')
    parser.add_argument('--table', required=True,
                        help='The name of the table to check consistency for')
    parser.add_argument('--result-column', required=True,
                        help='The name of the column with the results to check consistency for')
    args = parser.parse_args()

    if check_consistency(args.database, args.table, args.result_column):
        print(f'The {args.result_column} column in the {args.table} table is consistent')
    else:
        print(f'The {args.result_column} column in the {args.table} table is not consistent')
