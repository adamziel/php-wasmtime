
export function substring(str: string, start: i32, end: i32): string {
    return str.substring(start, end);
}
export function hello(str: string): string {
    return "Hello, world!" + str.length.toString();
}
export function hello_number(num: i32): string {
	console.log("num");
    return "Hello, world!" + num.toString();
}