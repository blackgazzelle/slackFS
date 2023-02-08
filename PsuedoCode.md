# Stripping file and Hiding

```
Input: Map file, disk file, List of cover files
Output: Stripped disk file and filled map file

# First the null bytes are stripped from the file to save space
stripped_file := strip_file(map_file, disk_file)

# Next the file is encoded using erasure coding creating multiple fragments
encoded_fragments := encode_file(stripped_file, ErasureCodingParameters)

# For each fragment we find a cover file that has enough slack space to hold the fragment
for fragment in encoded_fragments:
    cover_file := get_cover_file(list_cover_files)
    while size(cover_file) < size(fragment):
        cover_file := get_cover_file(list_cover_files)

    # Write each fragment to a different cover file perserving parity and data recovery
    slack_space := get_location_slack(cover_file)
    write(fragment, slack_space)

# Retrieving and restoring file
Input: Map file, disk file, List of cover files, number of fragments
Output: Recovered Disk file from slack space

# For each fragment we find a cover file that has enough slack space to hold the fragment
for fragment in number of fragments:
    cover_file := get_cover_file(list_cover_files)
    while size(cover_file) < size(fragment):
        cover_file := get_cover_file(list_cover_files)

    # read each fragment from their respective cover files
    slack_space = get_location_slack(cover_file)
    fragment := read(slack_space)
    fragments.append(frag)

# Now deocde the fragments using Erasure Coding and it will return the file that had its null bytes stripped
retrived_file := decode_fragments(fragments, ErasureCodingParameters)

# Now restore the null bytes resulting in the recovvered disk file
recovered_file := restore_null(retrieved_file, map_file)
```