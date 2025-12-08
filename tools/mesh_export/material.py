import argparse
import entities.material
import entities.serialize

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        prog='Material Writer',
        description='Converts a json material into a material binary'
    )

    parser.add_argument('input')
    parser.add_argument('output')
    parser.add_argument('-d', '--default')

    args = parser.parse_args()

    default_material = None

    if args.default != args.input and args.default:
        default_material = entities.material.parse_material(args.default)

    print(f'Writing material to {args.output}')
    result = entities.material.parse_material(args.input)
    print(result)
    entities.serialize.serialize_material(args.output, result)
